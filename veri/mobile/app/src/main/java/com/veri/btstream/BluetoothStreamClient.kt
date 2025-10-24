package com.veri.btstream

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothSocket
import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import org.tensorflow.lite.support.image.TensorImage
import org.tensorflow.lite.task.vision.detector.ObjectDetector
import java.io.InputStream
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.util.UUID
import kotlin.math.max
import kotlin.system.measureTimeMillis

private const val TAG = "BtStreamClient"
private const val HANDSHAKE_MAGIC = 0xCAFEBABEL.toInt()
private const val ACK_MAGIC = 0xABCD4321L
private const val HEADER_LEN = 4 + 4 + 4 + 2 // magic + size + timestamp + crc16
private val SPP_UUID: UUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB")

data class DetectionResult(
    val label: String,
    val confidence: Float,
    val description: String,
    val timestampMs: Long
)

/**
 * Handles Bluetooth SPP connection, handshake, frame decoding and TensorFlow Lite inference.
 * Designed to keep latency under 2 seconds for QQVGA JPEG frames.
 */
class BluetoothStreamClient(
    private val context: Context,
    private val detector: ObjectDetector,
    private val descriptionMap: Map<String, String>,
    private val scope: CoroutineScope
) {

    private var socket: BluetoothSocket? = null
    private var readerJob: Job? = null
    private var reconnectJob: Job? = null
    private var lastDevice: BluetoothDevice? = null

    fun connect(device: BluetoothDevice, onDetection: (DetectionResult) -> Unit) {
        lastDevice = device
        reconnectJob?.cancel()
        reconnectJob = scope.launch(Dispatchers.IO) {
            var attempt = 0
            while (isActive) {
                try {
                    Log.i(TAG, "Connecting to ${device.name} (${device.address}) attempt ${attempt + 1}")
                    val btSocket = device.createRfcommSocketToServiceRecord(SPP_UUID)
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery()
                    btSocket.connect()
                    socket = btSocket
                    Log.i(TAG, "Connected, running handshake")
                    performHandshake(btSocket.inputStream, btSocket.outputStream)
                    Log.i(TAG, "Handshake OK, starting reader loop")
                    startReaderLoop(btSocket.inputStream, btSocket.outputStream, onDetection)
                    return@launch
                } catch (ex: Exception) {
                    Log.w(TAG, "Connection attempt failed: ${ex.message}")
                    closeSilently()
                    attempt++
                    val backoff = max(1_000L, 2_000L * attempt)
                    delay(backoff.coerceAtMost(10_000L))
                }
            }
        }
    }

    fun disconnect() {
        readerJob?.cancel()
        reconnectJob?.cancel()
        closeSilently()
    }

    fun reconnectIfNeeded(onDetection: (DetectionResult) -> Unit) {
        val device = lastDevice ?: return
        if (socket?.isConnected == true) return
        connect(device, onDetection)
    }

    private fun performHandshake(input: InputStream, output: OutputStream) {
        val buffer = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN)
        buffer.putInt(HANDSHAKE_MAGIC)
        output.write(buffer.array())
        output.flush()

        val ackBytes = ByteArray(4)
        readFully(input, ackBytes, 0, ackBytes.size, 5_000)
        val ack = ByteBuffer.wrap(ackBytes).order(ByteOrder.LITTLE_ENDIAN).int
        if (ack.toLong() != ACK_MAGIC) {
            throw IllegalStateException("Unexpected ACK: 0x${ack.toString(16)}")
        }
    }

    private fun startReaderLoop(
        input: InputStream,
        output: OutputStream,
        onDetection: (DetectionResult) -> Unit
    ) {
        readerJob?.cancel()
        readerJob = scope.launch(Dispatchers.IO) {
            val header = ByteArray(HEADER_LEN)
            while (isActive) {
                try {
                    readFully(input, header, 0, HEADER_LEN, 5_000)
                    val buffer = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN)
                    val magic = buffer.int
                    if (magic != HANDSHAKE_MAGIC) {
                        Log.w(TAG, "Unexpected frame magic 0x${magic.toString(16)}; restarting")
                        throw IllegalStateException("bad magic")
                    }

                    val frameSize = buffer.int
                    val ts = buffer.int.toLong() and 0xFFFFFFFFL
                    val crcExpected = buffer.short.toInt() and 0xFFFF

                    val jpegBuffer = ByteArray(frameSize)
                    readFully(input, jpegBuffer, 0, frameSize, 2_000)
                    val crcActual = crc16Ccitt(jpegBuffer)
                    if (crcActual != crcExpected) {
                        Log.w(TAG, "CRC mismatch (exp=$crcExpected act=$crcActual)")
                        continue
                    }

                    var detectionResult: DetectionResult? = null
                    val latencyMs = measureTimeMillis {
                        val bitmap = decodeBitmap(jpegBuffer) ?: return@measureTimeMillis
                        detectionResult = runInference(bitmap, ts)
                    }

                    detectionResult?.let { result ->
                        Log.i(TAG, "Detection ${result.label} ${(result.confidence * 100).toInt()}% latency=${latencyMs}ms")
                        onDetection(result)
                        output.write("${result.description}\n".toByteArray())
                        output.flush()
                    } ?: Log.i(TAG, "No object detected")

                    if (latencyMs > 2_000) {
                        Log.w(TAG, "Latency ${latencyMs}ms exceeds SLA")
                    }
                } catch (ex: Exception) {
                    Log.w(TAG, "Reader loop error: ${ex.message}")
                    break
                }
            }

            closeSilently()
            if (isActive) {
                Log.i(TAG, "Attempting reconnection")
                delay(1_000)
                reconnectIfNeeded(onDetection)
            }
        }
    }

    private fun runInference(bitmap: Bitmap, frameTimestamp: Long): DetectionResult? {
        val tensorImage = TensorImage.fromBitmap(bitmap)
        val results = detector.detect(tensorImage)
        val best = results.maxByOrNull { detection ->
            detection.categories.maxOf { it.score }
        } ?: return null
        val category = best.categories.maxByOrNull { it.score } ?: return null
        val desc = descriptionMap[category.label] ?: "Objeto ${category.label}"
        return DetectionResult(
            label = category.label,
            confidence = category.score,
            description = "$desc (confiança ${(category.score * 100).toInt()}%)",
            timestampMs = frameTimestamp
        )
    }

    private fun decodeBitmap(jpeg: ByteArray): Bitmap? = BitmapFactory.decodeByteArray(jpeg, 0, jpeg.size)

    private fun closeSilently() {
        try {
            socket?.close()
        } catch (_: Exception) {
        }
        socket = null
    }

    companion object {
        private fun readFully(
            input: InputStream,
            buffer: ByteArray,
            offset: Int,
            count: Int,
            timeoutMs: Long
        ) {
            var remaining = count
            var position = offset
            val deadline = System.currentTimeMillis() + timeoutMs
            while (remaining > 0) {
                if (System.currentTimeMillis() > deadline) {
                    throw IllegalStateException("Timeout reading stream ($count bytes)")
                }
                val read = input.read(buffer, position, remaining)
                if (read == -1) throw IllegalStateException("Stream closed")
                remaining -= read
                position += read
            }
        }

        private fun crc16Ccitt(data: ByteArray): Int {
            var crc = 0xFFFF
            for (b in data) {
                crc = crc xor ((b.toInt() and 0xFF) shl 8)
                repeat(8) {
                    crc = if ((crc and 0x8000) != 0) {
                        (crc shl 1) xor 0x1021
                    } else {
                        crc shl 1
                    }
                }
                crc = crc and 0xFFFF
            }
            return crc
        }
    }
}
