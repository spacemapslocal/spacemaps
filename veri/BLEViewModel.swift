import CoreBluetooth
import CoreML
import Combine
import SwiftUI
import Vision

final class BLEViewModel: NSObject, ObservableObject {
    @Published var statusText: String = "Aguardando conexão..."
    @Published var lastDescription: String = ""
    @Published var latencyMs: Double = 0

    private let serviceUUID = CBUUID(string: "FFE0")
    private let frameUUID = CBUUID(string: "FFE1")
    private let commandUUID = CBUUID(string: "FFE2")

    private let handshakeMagic: UInt32 = 0xCAFEBABE
    private let ackMagic: UInt32 = 0xABCD4321

    private lazy var central: CBCentralManager = CBCentralManager(delegate: self, queue: .main)
    private var peripheral: CBPeripheral?
    private var frameCharacteristic: CBCharacteristic?
    private var commandCharacteristic: CBCharacteristic?

    private var expectedLength: Int = 0
    private var expectedCRC: UInt16 = 0
    private var frameTimestamp: UInt32 = 0
    private var buffer = Data()
    private var handshakeSent = false

    private let model: VNCoreMLModel? = {
        guard let mlModel = try? SSDModel(configuration: MLModelConfiguration()).model else {
            return nil
        }
        return try? VNCoreMLModel(for: mlModel)
    }()

    func connect() {
        statusText = "Inicializando Bluetooth..."
        if central.state == .poweredOn {
            startScan()
        }
    }

    private func startScan() {
        statusText = "Procurando ESP32CAM-BLE..."
        central.scanForPeripherals(withServices: [serviceUUID], options: nil)
    }

    private func sendHandshake() {
        guard let characteristic = commandCharacteristic, let peripheral = peripheral else { return }
        var magic = handshakeMagic.littleEndian
        let data = Data(bytes: &magic, count: MemoryLayout.size(ofValue: magic))
        peripheral.writeValue(data, for: characteristic, type: .withoutResponse)
        handshakeSent = true
        statusText = "Handshake enviado..."
    }

    private func resetFrame() {
        expectedLength = 0
        expectedCRC = 0
        frameTimestamp = 0
        buffer.removeAll(keepingCapacity: true)
    }

    private func handleHeader(_ data: Data) {
        guard data.count == 14 else { return }
        var cursor = data
        let magic = cursor.readUInt32()
        guard magic == handshakeMagic else {
            statusText = "Header inválido"
            resetFrame()
            return
        }
        expectedLength = Int(cursor.readUInt32())
        frameTimestamp = cursor.readUInt32()
        expectedCRC = cursor.readUInt16()
        buffer.removeAll(keepingCapacity: true)
    }

    private func handleFrameChunk(_ data: Data) {
        guard expectedLength > 0 else {
            handleHeader(data)
            return
        }

        buffer.append(data)
        if buffer.count >= expectedLength {
            finalizeFrame()
        }
    }

    private func finalizeFrame() {
        guard buffer.count >= expectedLength else { return }
        let frameData = buffer.prefix(expectedLength)
        let crc = crc16Ccitt(frameData)
        guard crc == expectedCRC else {
            statusText = "CRC inválido, descartado"
            resetFrame()
            return
        }

        let start = CFAbsoluteTimeGetCurrent()
        if let result = detectObjects(from: Data(frameData)) {
            let elapsed = (CFAbsoluteTimeGetCurrent() - start) * 1_000
            latencyMs = elapsed
            lastDescription = result
            statusText = "Detecção: \(result)"
        } else {
            statusText = "Sem objeto confiável"
        }
        resetFrame()
    }

    private func detectObjects(from data: Data) -> String? {
        guard
            let image = UIImage(data: data),
            let cgImage = image.cgImage,
            let model = model
        else { return nil }

        let handler = VNImageRequestHandler(cgImage: cgImage, options: [:])
        var description: String?

        let request = VNCoreMLRequest(model: model) { request, _ in
            guard let results = request.results as? [VNRecognizedObjectObservation],
                  let best = results.max(by: { ($0.confidence) < ($1.confidence) }),
                  let label = best.labels.first
            else { return }

            let confidence = Int(label.confidence * 100)
            description = "\(label.identifier) (confiança \(confidence)%)"
        }
        request.imageCropAndScaleOption = .scaleFill

        try? handler.perform([request])
        return description
    }

    private func crc16Ccitt(_ data: Data) -> UInt16 {
        var crc: UInt16 = 0xFFFF
        for byte in data {
            crc ^= UInt16(byte) << 8
            for _ in 0..<8 {
                if (crc & 0x8000) != 0 {
                    crc = (crc << 1) ^ 0x1021
                } else {
                    crc <<= 1
                }
            }
        }
        return crc
    }
}

extension BLEViewModel: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            startScan()
        } else {
            statusText = "Bluetooth indisponível (\(central.state.rawValue))"
        }
    }

    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String: Any], rssi RSSI: NSNumber) {
        statusText = "Encontrado \(peripheral.name ?? "desconhecido"), conectando..."
        self.peripheral = peripheral
        central.stopScan()
        central.connect(peripheral, options: nil)
    }

    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        statusText = "Conectado, descobrindo serviços..."
        peripheral.delegate = self
        peripheral.discoverServices([serviceUUID])
    }

    func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        statusText = "Falha na conexão: \(error?.localizedDescription ?? "desconhecida")"
        startScan()
    }

    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        statusText = "Desconectado, tentando reconectar..."
        handshakeSent = false
        resetFrame()
        startScan()
    }
}

extension BLEViewModel: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        if let error = error {
            statusText = "Erro serviços: \(error.localizedDescription)"
            return
        }
        guard let services = peripheral.services else { return }
        for service in services where service.uuid == serviceUUID {
            peripheral.discoverCharacteristics([frameUUID, commandUUID], for: service)
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        if let error = error {
            statusText = "Erro chars: \(error.localizedDescription)"
            return
        }
        guard let characteristics = service.characteristics else { return }
        for characteristic in characteristics {
            if characteristic.uuid == frameUUID {
                frameCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
            } else if characteristic.uuid == commandUUID {
                commandCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
            }
        }
        if !handshakeSent {
            sendHandshake()
        }
    }

    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        if let error = error {
            statusText = "Erro atualização: \(error.localizedDescription)"
            return
        }
        guard let data = characteristic.value else { return }

        if characteristic.uuid == commandUUID {
            if data.count >= 4 {
                var cursor = data
                let value = cursor.readUInt32()
                if value == ackMagic {
                    statusText = "Handshake confirmado! Recebendo frames..."
                }
            }
            return
        }

        if characteristic.uuid == frameUUID {
            handleFrameChunk(data)
        }
    }
}

private extension Data {
    mutating func readUInt32() -> UInt32 {
        let value = withUnsafeBytes { $0.load(as: UInt32.self) }
        removeFirst(4)
        return UInt32(littleEndian: value)
    }

    mutating func readUInt16() -> UInt16 {
        let value = withUnsafeBytes { $0.load(as: UInt16.self) }
        removeFirst(2)
        return UInt16(littleEndian: value)
    }
}
