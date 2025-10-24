#include "esp_camera.h"
#include "BluetoothSerial.h"
#include "esp_timer.h"

// Camera model configuration (AI Thinker default)
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// Bluetooth configuration
static BluetoothSerial SerialBT;
static constexpr char kBtDeviceName[] = "ESP32CAM-Detector";
static constexpr uint32_t kHandshakeMagic = 0xCAFEBABE;
static constexpr uint32_t kAckMagic = 0xABCD4321;

// Timing targets (milliseconds)
static constexpr uint32_t kReconnectMaxMs = 10'000;
static constexpr uint32_t kCaptureIntervalMs = 500;  // ≈2 fps for latency targets

// Frame settings
static constexpr framesize_t kFrameSize = FRAMESIZE_QQVGA;  // 160x120
static constexpr uint8_t kJpegQuality = 12;  // Higher compression (lower quality number)

// Helper structure for packets
struct __attribute__((packed)) FrameHeader {
  uint32_t magic;
  uint32_t frameSize;
  uint32_t timestampMs;
  uint16_t crc16;
};

// Forward declarations
bool initCamera();
uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc = 0xFFFF);
void waitForClient();
bool ensureHandshake();
bool sendFrame(camera_fb_t* fb);
void sendHeartbeat(uint32_t nowMs);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  if (!initCamera()) {
    Serial.println("[ERR] Camera init failed");
    esp_restart();
  }

  if (!SerialBT.begin(kBtDeviceName)) {
    Serial.println("[ERR] Bluetooth init failed");
    esp_restart();
  }
  Serial.printf("[INF] Bluetooth device ready as '%s'\n", kBtDeviceName);
}

void loop() {
  waitForClient();
  if (!ensureHandshake()) {
    delay(200);
    return;
  }

  const uint32_t nowMs = millis();
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("[ERR] Failed to capture frame");
    delay(200);
    return;
  }

  const bool sent = sendFrame(fb);
  esp_camera_fb_return(fb);

  if (!sent) {
    Serial.println("[WRN] Failed to send frame, dropping connection");
    SerialBT.disconnect();
    delay(200);
    return;
  }

  sendHeartbeat(nowMs);

  // Throttle capture to maintain processing budget on mobile device
  const uint32_t elapsed = millis() - nowMs;
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

  if (psramFound()) {
    config.frame_size = kFrameSize;
    config.jpeg_quality = kJpegQuality;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = kFrameSize;
    config.jpeg_quality = kJpegQuality;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

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

void waitForClient() {
  static uint32_t lastAdvertiseMs = 0;
  static uint32_t lastRestartMs = 0;

  if (SerialBT.hasClient()) {
    return;
  }

  const uint32_t now = millis();
  if (now - lastRestartMs > kReconnectMaxMs) {
    Serial.println("[INF] Resetting Bluetooth service to encourage reconnection");
    SerialBT.end();
    if (!SerialBT.begin(kBtDeviceName)) {
      Serial.println("[ERR] Bluetooth restart failed");
    }
    lastRestartMs = now;
  }

  while (!SerialBT.hasClient()) {
    const uint32_t tick = millis();
    if (tick - lastAdvertiseMs > 1'000) {
      Serial.println("[INF] Waiting for Bluetooth client...");
      lastAdvertiseMs = tick;
    }
    delay(200);
  }
}

bool ensureHandshake() {
  static bool handshakeDone = false;

  if (!SerialBT.hasClient()) {
    handshakeDone = false;
    return false;
  }

  if (handshakeDone) {
    return true;
  }

  Serial.println("[INF] Client connected, running handshake");
  uint32_t start = millis();
  while (SerialBT.available() < sizeof(uint32_t)) {
    if (millis() - start > 5'000) {
      Serial.println("[WRN] Handshake timeout, forcing disconnect");
      SerialBT.disconnect();
      handshakeDone = false;
      return false;
    }
    delay(10);
  }

  uint32_t magic = 0;
  SerialBT.readBytes(reinterpret_cast<uint8_t*>(&magic), sizeof(magic));
  if (magic != kHandshakeMagic) {
    Serial.printf("[WRN] Unexpected handshake value 0x%08lx\n", magic);
    SerialBT.disconnect();
    handshakeDone = false;
    return false;
  }
  SerialBT.write(reinterpret_cast<const uint8_t*>(&kAckMagic), sizeof(kAckMagic));
  Serial.println("[INF] Handshake completed");
  handshakeDone = true;
  return true;
}

bool sendFrame(camera_fb_t* fb) {
  if (!SerialBT.hasClient()) {
    return false;
  }

  FrameHeader header{};
  header.magic = kHandshakeMagic;
  header.frameSize = fb->len;
  header.timestampMs = millis();
  header.crc16 = crc16_ccitt(fb->buf, fb->len);

  const size_t bytesWritten = SerialBT.write(reinterpret_cast<uint8_t*>(&header), sizeof(header));
  if (bytesWritten != sizeof(header)) {
    Serial.println("[ERR] Failed to write header");
    return false;
  }

  const size_t frameWritten = SerialBT.write(fb->buf, fb->len);
  if (frameWritten != fb->len) {
    Serial.printf("[ERR] Frame write mismatch (%u/%u)\n", static_cast<unsigned>(frameWritten),
                  static_cast<unsigned>(fb->len));
    return false;
  }

  Serial.printf("[DBG] Frame %u bytes sent\n", static_cast<unsigned>(fb->len));
  return true;
}

void sendHeartbeat(uint32_t nowMs) {
  static uint32_t lastHeartbeat = 0;
  if (nowMs - lastHeartbeat < 1'000) {
    return;
  }
  lastHeartbeat = nowMs;

  const char* message = "HB\n";
  SerialBT.write(reinterpret_cast<const uint8_t*>(message), 3);
}

uint16_t crc16_ccitt(const uint8_t* data, size_t len, uint16_t crc) {
  while (len--) {
    crc ^= static_cast<uint16_t>(*data++) << 8;
    for (int i = 0; i < 8; ++i) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}
