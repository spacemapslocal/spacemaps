package com.veri.btstream

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.os.Bundle
import android.widget.Button
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.lifecycle.lifecycleScope
import org.tensorflow.lite.task.vision.detector.ObjectDetector
import org.tensorflow.lite.task.vision.detector.ObjectDetector.ObjectDetectorOptions

class MainActivity : ComponentActivity() {

    private lateinit var streamClient: BluetoothStreamClient
    private lateinit var statusView: TextView
    private lateinit var connectButton: Button

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        statusView = findViewById(R.id.statusText)
        connectButton = findViewById(R.id.connectButton)

        val detectorOptions = ObjectDetectorOptions.builder()
            .setScoreThreshold(0.5f)
            .setMaxResults(3)
            .build()
        val detector = ObjectDetector.createFromFileAndOptions(
            this,
            "ssd_mobilenet_v1.tflite",
            detectorOptions
        )

        val descriptions = mapOf(
            "person" to "Pessoa detectada.",
            "cup" to "Copo: recipiente para líquidos.",
            "bottle" to "Garrafa identificada.",
            "cell phone" to "Telefone celular presente."
        )

        streamClient = BluetoothStreamClient(
            context = this,
            detector = detector,
            descriptionMap = descriptions,
            scope = lifecycleScope
        )

        connectButton.setOnClickListener {
            val device = pickEspDevice() ?: return@setOnClickListener
            statusView.text = "Conectando a ${device.name}..."
            streamClient.connect(device) { result ->
                runOnUiThread {
                    statusView.text = "Objeto: ${result.description}"
                }
            }
        }
    }

    override fun onDestroy() {
        streamClient.disconnect()
        super.onDestroy()
    }

    private fun pickEspDevice(): BluetoothDevice? {
        val adapter = BluetoothAdapter.getDefaultAdapter() ?: return null
        return adapter.bondedDevices.firstOrNull { it.name == "ESP32CAM-Detector" }
    }
}
