#include "esp_camera.h"
#include "NimBLEDevice.h"

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// BLE UUIDs (custom UART-like service)
static constexpr char kServiceUuid[] = "0000FFE0-0000-1000-8000-00805F9B34FB";
static constexpr char kFrameCharUuid[] = "0000FFE1-0000-1000-8000-00805F9B34FB";
static constexpr char kCommandCharUuid[] = "0000FFE2-0000-1000-8000-00805F9B34FB";

// Handshake tokens
static constexpr uint32_t kHandshakeMagic = 0xCAFEBABE;
static constexpr uint32_t kAckMagic = 0xABCD4321;

// Frame parameters
static constexpr framesize_t kFrameSize = FRAMESIZE_QQVGA;
static constexpr uint8_t kJpegQuality = 12;
static constexpr uint16_t kBleChunkLen = 244;  // safe payload under common MTU
static constexpr uint32_t kCaptureIntervalMs = 600;

// Packet header (sent as first BLE chunk)
struct __attribute__((packed)) FrameHeader {
  uint32_t magic;
  uint32_t frameSize;
  uint32_t timestampMs;
  uint16_t crc16;
};

// State
static NimBLECharacteristic* frameChar = nullptr;
static NimBLECharacteristic* commandChar = nullptr;
static volatile bool clientConnected = false;
static volatile bool handshakeComplete = false;

// Forward declarations
bool initCamera();
uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF);
void sendFrame(camera_fb_t* fb);
void sendAck();

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* server) override {
    clientConnected = true;
    server->updateConnParams(server->getPeerID(), 24, 40, 0, 400);  // tune connection interval
    Serial.println("[INF] BLE client connected");
  }

  void onDisconnect(NimBLEServer* server) override {
    Serial.println("[INF] BLE client disconnected");
    clientConnected = false;
    handshakeComplete = false;
    NimBLEDevice::startAdvertising();
  }
};

class CommandCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* characteristic) override {
    std::string value = characteristic->getValue();
    if (value.size() < sizeof(uint32_t)) {
      return;
    }

    uint32_t token = 0;
    memcpy(&token, value.data(), sizeof(uint32_t));
    if (token == kHandshakeMagic) {
      Serial.println("[INF] Handshake token received");
      handshakeComplete = true;
      sendAck();
    } else {
      Serial.printf("[DBG] Command token 0x%08lx\n", token);
    }
  }
};

void setup() {
  Serial.begin(115200);
  if (!initCamera()) {
    Serial.println("[ERR] Camera init failed");
    esp_restart();
  }

  NimBLEDevice::init("ESP32CAM-BLE");
  NimBLEDevice::setMTU(517);
  NimBLEDevice::setPower(ESP_PWR_LVL_P7);

  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());

  NimBLEService* service = server->createService(kServiceUuid);

  frameChar = service->createCharacteristic(
      kFrameCharUuid,
      NIMBLE_PROPERTY::NOTIFY
  );

  commandChar = service->createCharacteristic(
      kCommandCharUuid,
      NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY
  );
  commandChar->setCallbacks(new CommandCallbacks());

  service->start();
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("[INF] BLE advertising started");
}

void loop() {
  if (!clientConnected || !handshakeComplete) {
    delay(200);
    return;
  }

  const uint32_t frameStart = millis();
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERR] Capture failed");
    delay(200);
    return;
  }

  sendFrame(fb);
  esp_camera_fb_return(fb);

  const uint32_t elapsed = millis() - frameStart;
  if (elapsed < kCaptureIntervalMs) {
    delay(kCaptureIntervalMs - elapsed);
  }
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20'000'000;
  config.pixel_format = PIXFORMAT_JPEG;

  config.frame_size = kFrameSize;
  config.jpeg_quality = kJpegQuality;
  config.fb_count = psramFound() ? 2 : 1;
  config.fb_location = psramFound() ? CAMERA_FB_IN_PSRAM : CAMERA_FB_IN_DRAM;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERR] Camera init failed: 0x%x\n", err);
    return false;
  }

  sensor_t* sensor = esp_camera_sensor_get();
  sensor->set_framesize(sensor, kFrameSize);
  sensor->set_quality(sensor, kJpegQuality);
  return true;
}

void sendFrame(camera_fb_t* fb) {
  if (!frameChar) return;

  FrameHeader header{};
  header.magic = kHandshakeMagic;
  header.frameSize = fb->len;
  header.timestampMs = millis();
  header.crc16 = crc16_ccitt(fb->buf, fb->len);

  // Send header first
  frameChar->setValue(reinterpret_cast<uint8_t*>(&header), sizeof(header));
  frameChar->notify();

  // Chunk JPEG buffer
  size_t offset = 0;
  while (offset < fb->len) {
    const size_t remaining = fb->len - offset;
    const size_t chunk = remaining > kBleChunkLen ? kBleChunkLen : remaining;
    frameChar->setValue(fb->buf + offset, chunk);
    frameChar->notify();
    offset += chunk;
    delay(6);  // allow controller to flush notifications
  }

  Serial.printf("[DBG] Frame %u bytes sent via BLE\n", static_cast<unsigned>(fb->len));
}

void sendAck() {
  if (!commandChar) return;
  commandChar->setValue(reinterpret_cast<const uint8_t*>(&kAckMagic), sizeof(kAckMagic));
  commandChar->notify();
}

uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc) {
  while (len--) {
    crc ^= static_cast<uint16_t>(*data++) << 8;
    for (int i = 0; i < 8; ++i) {
      crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
    }
  }
  return crc;
}
