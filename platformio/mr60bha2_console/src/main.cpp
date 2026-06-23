/*
 * MR60BHA2 self-contained device console
 *
 * PlatformIO Arduino firmware provides:
 *   - MR60BHA2 heart, breathing, range, presence, and raw phase readings
 *   - BH1750 ambient-light readings over I2C
 *   - WS2812 RGB control and sensor threshold rules
 *   - A compact dashboard served directly by the XIAO ESP32-C6
 *
 * Upload firmware and filesystem, connect to WiFi "mmWaveVisLog-MR60BHA2" (password "wirelessphysiology"), and open:
 *   http://192.168.4.1/
 */

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <HardwareSerial.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Wire.h>
#include "esp32-hal-rgb-led.h"
#include "Seeed_Arduino_mmWave.h"

static constexpr int MMWAVE_RX_PIN = 17;
static constexpr int MMWAVE_TX_PIN = 16;
static constexpr int RGB_LED_PIN = 1;
static constexpr int I2C_SDA_PIN = 22;
static constexpr int I2C_SCL_PIN = 23;
static constexpr uint8_t BH1750_ADDRESS = 0x23;
static constexpr float MAX_VALID_DISTANCE_M = 6.0f;
static constexpr uint8_t MAX_UI_TARGETS = MAX_TARGET_NUM;
static const char *UI_INDEX_PATH = "/index.html";

static const char *WIFI_AP_SSID = "mmWaveVisLog-MR60BHA2";
static const char *WIFI_AP_PASSWORD = "wirelessphysiology";
static const char *OTA_HOSTNAME = "mmWaveVisLog-MR60BHA2-OTA";
static const char *OTA_PASSWORD = "wp-ota";
static const char *VisLog_FW_VERSION = "2.1.4";

HardwareSerial mmWaveSerial(1);
SEEED_MR60BHA2 mmWave;
WebServer server(80);

struct TargetSample {
  float x = NAN;
  float y = NAN;
  float distance = NAN;
  float angle = NAN;
  float speed = NAN;
  int32_t dopIndex = 0;
  int32_t clusterIndex = 0;
};

struct Sample {
  float heartRate = NAN;
  float breathRate = NAN;
  float distance = NAN;
  float rawDistance = NAN;
  float totalPhase = NAN;
  float breathPhase = NAN;
  float heartPhase = NAN;
  float light = NAN;
  float targetX = NAN;
  float targetY = NAN;
  float targetDistance = NAN;
  float targetAngle = NAN;
  float targetSpeed = NAN;
  TargetSample targets[MAX_UI_TARGETS];
  uint8_t targetCount = 0;
  bool targetValid = false;
  bool targetInfoValid = false;
  bool pointCloudValid = false;
  bool presence = false;
  bool presenceValid = false;
  String presenceSource = "waiting";
  String targetSource = "waiting";
  uint32_t firmwareRaw = 0;
  uint8_t firmwareProject = 0;
  uint8_t firmwareMajor = 0;
  uint8_t firmwareSub = 0;
  uint8_t firmwareModified = 0;
  bool firmwareValid = false;
  uint8_t ledR = 0;
  uint8_t ledG = 0;
  uint8_t ledB = 0;
  uint32_t frame = 0;
  uint32_t lastRadarMs = 0;
  uint32_t lastPresenceMs = 0;
  uint32_t lastTargetMs = 0;
  uint32_t lastDetectionSignalMs = 0;
  uint32_t lastFirmwareMs = 0;
};

enum class LedMode : uint8_t { Off, Solid, Pulse, Rule };

struct LedState {
  LedMode mode = LedMode::Solid;
  float brightness = 0.35f;
  uint8_t r = 40;
  uint8_t g = 190;
  uint8_t b = 100;
  String metric = "target_speed";
  float low = -0.05f;
  float high = 0.05f;
  uint8_t lowR = 255, lowG = 0, lowB = 0;
  uint8_t okR = 0, okG = 0, okB = 0;
  uint8_t highR = 0, highG = 80, highB = 255;
};

Sample sample;
LedState led;
bool lightSensorReady = false;
uint32_t lastLightReadMs = 0;
uint32_t lastSerialReportMs = 0;

String jsonNumber(float value, unsigned int digits) {
  if (!isfinite(value)) return "null";
  return String(value, digits);
}

String readJsonString(const String &body, const char *key, const String &fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  int start = body.indexOf('"', colonPos + 1);
  int end = body.indexOf('"', start + 1);
  return start >= 0 && end > start ? body.substring(start + 1, end) : fallback;
}

float readJsonFloat(const String &body, const char *key, float fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;
  return body.substring(colonPos + 1).toFloat();
}

int readJsonInt(const String &body, const char *key, int fallback) {
  return int(readJsonFloat(body, key, float(fallback)));
}

void writeLed(uint8_t r, uint8_t g, uint8_t b, float brightness) {
  brightness = constrain(brightness, 0.0f, 1.0f);
  sample.ledR = uint8_t(r * brightness);
  sample.ledG = uint8_t(g * brightness);
  sample.ledB = uint8_t(b * brightness);
  rgbLedWrite(RGB_LED_PIN, sample.ledR, sample.ledG, sample.ledB);
}

float metricValue(const String &metric) {
  if (metric == "heart_rate") return sample.heartRate;
  if (metric == "breath_rate") return sample.breathRate;
  if (metric == "distance") return sample.distance;
  if (metric == "raw_distance") return sample.rawDistance;
  if (metric == "presence") return sample.presenceValid ? (sample.presence ? 1.0f : 0.0f) : NAN;
  if (metric == "people_count") return sample.targetInfoValid || sample.pointCloudValid ? float(sample.targetCount) : NAN;
  if (metric == "target_x") return sample.targetX;
  if (metric == "target_y") return sample.targetY;
  if (metric == "target_angle") return sample.targetAngle;
  if (metric == "target_speed") return sample.targetSpeed;
  if (metric == "light") return sample.light;
  if (metric == "total_phase") return sample.totalPhase;
  if (metric == "breath_phase") return sample.breathPhase;
  if (metric == "heart_phase") return sample.heartPhase;
  return NAN;
}

void updateLed() {
  if (led.mode == LedMode::Off) {
    writeLed(0, 0, 0, 1.0f);
    return;
  }
  if (led.mode == LedMode::Pulse) {
    float wave = (sinf(float(millis()) * 0.004f) + 1.0f) * 0.5f;
    writeLed(led.r, led.g, led.b, led.brightness * (0.12f + 0.88f * wave));
    return;
  }
  if (led.mode == LedMode::Rule) {
    float value = metricValue(led.metric);
    if (!isfinite(value)) {
      writeLed(20, 20, 20, led.brightness);
    } else if (value < led.low) {
      writeLed(led.lowR, led.lowG, led.lowB, led.brightness);
    } else if (value > led.high) {
      writeLed(led.highR, led.highG, led.highB, led.brightness);
    } else {
      writeLed(led.okR, led.okG, led.okB, led.brightness);
    }
    return;
  }
  writeLed(led.r, led.g, led.b, led.brightness);
}

bool beginBh1750() {
  Wire.beginTransmission(BH1750_ADDRESS);
  Wire.write(0x01);  // Power on.
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(BH1750_ADDRESS);
  Wire.write(0x10);  // Continuous high-resolution mode.
  return Wire.endTransmission() == 0;
}

void updateLight() {
  if (!lightSensorReady || millis() - lastLightReadMs < 250) return;
  lastLightReadMs = millis();
  if (Wire.requestFrom(BH1750_ADDRESS, uint8_t(2)) != 2) return;
  uint16_t raw = (uint16_t(Wire.read()) << 8) | uint16_t(Wire.read());
  sample.light = float(raw) / 1.2f;
}

float normalizeDistance(float raw) {
  if (!isfinite(raw) || raw <= 0.0f) return NAN;
  // Some MR60BHA2 firmware revisions report centimetres despite the API's
  // metre examples. Values beyond the hardware range are treated as cm.
  float metres = raw > MAX_VALID_DISTANCE_M ? raw / 100.0f : raw;
  return metres >= 0.05f && metres <= MAX_VALID_DISTANCE_M ? metres : NAN;
}

float normalizePosition(float raw) {
  if (!isfinite(raw)) return NAN;
  return fabsf(raw) > 10.0f ? raw / 100.0f : raw;
}

void clearTargets() {
  sample.targetCount = 0;
  sample.targetValid = false;
  sample.targetX = NAN;
  sample.targetY = NAN;
  sample.targetDistance = NAN;
  sample.targetAngle = NAN;
  sample.targetSpeed = NAN;
  for (uint8_t i = 0; i < MAX_UI_TARGETS; i++) {
    sample.targets[i] = TargetSample();
  }
}

void storeTargets(const PeopleCounting &targets, const char *source) {
  clearTargets();
  sample.targetSource = source;
  size_t count = targets.targets.size();
  if (count > MAX_UI_TARGETS) count = MAX_UI_TARGETS;
  sample.targetCount = uint8_t(count);
  sample.targetValid = sample.targetCount > 0;
  for (uint8_t i = 0; i < sample.targetCount; i++) {
    const TargetN &target = targets.targets[i];
    TargetSample &out = sample.targets[i];
    out.x = normalizePosition(target.x_point);
    out.y = normalizePosition(target.y_point);
    out.distance = hypotf(out.x, out.y);
    out.angle = atan2f(out.x, out.y) * 180.0f / PI;
    out.speed = float(target.dop_index) * RANGE_STEP / 100.0f;
    out.dopIndex = target.dop_index;
    out.clusterIndex = target.cluster_index;
  }
  if (sample.targetValid) {
    sample.targetX = sample.targets[0].x;
    sample.targetY = sample.targets[0].y;
    sample.targetDistance = sample.targets[0].distance;
    sample.targetAngle = sample.targets[0].angle;
    sample.targetSpeed = sample.targets[0].speed;
    sample.lastDetectionSignalMs = millis();
  }
}

void updateDerivedPresence() {
  uint32_t now = millis();
  if (sample.lastTargetMs && now - sample.lastTargetMs > 1500) {
    clearTargets();
    sample.targetSource = "stale";
  }
  if (sample.lastPresenceMs && now - sample.lastPresenceMs <= 1200) {
    sample.presenceValid = true;
  } else {
    sample.presence = false;
    sample.presenceValid = false;
  }
  sample.presenceSource = sample.presenceValid ? "sensor" : "waiting";
}

bool refreshFirmwareInfo(bool force = false) {
  uint32_t now = millis();
  if (!force && sample.firmwareValid && now - sample.lastFirmwareMs < 5000) return false;
  FirmwareInfo firmware;
  if (mmWave.readFirmwareInfo(firmware) != seeed::mmwave::Status::Ok) return false;
  sample.firmwareRaw = firmware.value;
  sample.firmwareProject = firmware.firmware_verson.project_name;
  sample.firmwareMajor = firmware.firmware_verson.major_version;
  sample.firmwareSub = firmware.firmware_verson.sub_version;
  sample.firmwareModified = firmware.firmware_verson.modified_version;
  sample.firmwareValid = true;
  sample.lastFirmwareMs = now;
  return true;
}

void updateRadar() {
  if (!mmWave.update(20)) return;
  bool changed = false;

  float total, breath, heart;
  if (mmWave.getHeartBreathPhases(total, breath, heart)) {
    sample.totalPhase = total;
    sample.breathPhase = breath;
    sample.heartPhase = heart;
    changed = true;
  }

  float value;
  if (mmWave.getBreathRate(value)) {
    sample.breathRate = value > 0.0f && value < 80.0f ? value : NAN;
    if (isfinite(sample.breathRate)) sample.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (mmWave.getHeartRate(value)) {
    sample.heartRate = value > 0.0f && value < 240.0f ? value : NAN;
    if (isfinite(sample.heartRate)) sample.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (mmWave.getDistance(value)) {
    sample.rawDistance = value;
    sample.distance = normalizeDistance(value);
    if (isfinite(sample.distance)) sample.lastDetectionSignalMs = millis();
    changed = true;
  }

  bool detected;
  if (mmWave.readPresence(detected) == seeed::mmwave::Status::Ok) {
    sample.presence = detected;
    sample.presenceValid = true;
    sample.lastPresenceMs = millis();
    sample.presenceSource = "sensor";
    changed = true;
  }

  PeopleCounting targets;
  bool gotTargetInfo = false;
  if (mmWave.readTargetInfo(targets) == seeed::mmwave::Status::Ok) {
    gotTargetInfo = true;
    sample.lastTargetMs = millis();
    sample.targetInfoValid = true;
    storeTargets(targets, "target info");
    changed = true;
  }
  if (mmWave.readPointCloud(targets) == seeed::mmwave::Status::Ok) {
    sample.lastTargetMs = millis();
    sample.pointCloudValid = true;
    if (!gotTargetInfo) {
      storeTargets(targets, "point cloud");
    }
    changed = true;
  }

  if (refreshFirmwareInfo()) changed = true;

  updateDerivedPresence();

  if (changed) {
    sample.frame++;
    sample.lastRadarMs = millis();
  }
}

String sampleJson() {
  String json;
  json.reserve(1180);
  json = "{";
  json += "\"heart_rate\":" + jsonNumber(sample.heartRate, 2) + ",";
  json += "\"breath_rate\":" + jsonNumber(sample.breathRate, 2) + ",";
  json += "\"distance\":" + jsonNumber(sample.distance, 3) + ",";
  json += "\"raw_distance\":" + jsonNumber(sample.rawDistance, 3) + ",";
  json += "\"total_phase\":" + jsonNumber(sample.totalPhase, 3) + ",";
  json += "\"breath_phase\":" + jsonNumber(sample.breathPhase, 3) + ",";
  json += "\"heart_phase\":" + jsonNumber(sample.heartPhase, 3) + ",";
  json += "\"presence\":" + String(sample.presence ? "true" : "false") + ",";
  json += "\"presence_valid\":" + String(sample.presenceValid ? "true" : "false") + ",";
  json += "\"presence_source\":\"" + sample.presenceSource + "\",";
  json += "\"target_valid\":" + String(sample.targetValid ? "true" : "false") + ",";
  json += "\"target_source\":\"" + sample.targetSource + "\",";
  json += "\"people_count\":" + String(sample.targetCount) + ",";
  json += "\"target_count\":" + String(sample.targetCount) + ",";
  json += "\"target_x\":" + jsonNumber(sample.targetX, 3) + ",";
  json += "\"target_y\":" + jsonNumber(sample.targetY, 3) + ",";
  json += "\"target_distance\":" + jsonNumber(sample.targetDistance, 3) + ",";
  json += "\"target_angle\":" + jsonNumber(sample.targetAngle, 2) + ",";
  json += "\"target_speed\":" + jsonNumber(sample.targetSpeed, 3) + ",";
  json += "\"targets\":[";
  for (uint8_t i = 0; i < sample.targetCount; i++) {
    if (i) json += ",";
    json += "{";
    json += "\"x\":" + jsonNumber(sample.targets[i].x, 3) + ",";
    json += "\"y\":" + jsonNumber(sample.targets[i].y, 3) + ",";
    json += "\"distance\":" + jsonNumber(sample.targets[i].distance, 3) + ",";
    json += "\"angle\":" + jsonNumber(sample.targets[i].angle, 2) + ",";
    json += "\"speed\":" + jsonNumber(sample.targets[i].speed, 3) + ",";
    json += "\"dop_index\":" + String(sample.targets[i].dopIndex) + ",";
    json += "\"cluster_index\":" + String(sample.targets[i].clusterIndex);
    json += "}";
  }
  json += "],";
  json += "\"light\":" + jsonNumber(sample.light, 1) + ",";
  json += "\"light_ready\":" + String(lightSensorReady ? "true" : "false") + ",";
  json += "\"console_fw\":\"" + String(VisLog_FW_VERSION) + "\",";
  json += "\"firmware_valid\":" + String(sample.firmwareValid ? "true" : "false") + ",";
  json += "\"firmware_raw\":" + String(sample.firmwareRaw) + ",";
  json += "\"firmware_project\":" + String(sample.firmwareProject) + ",";
  json += "\"firmware_major\":" + String(sample.firmwareMajor) + ",";
  json += "\"firmware_sub\":" + String(sample.firmwareSub) + ",";
  json += "\"firmware_modified\":" + String(sample.firmwareModified) + ",";
  json += "\"led_r\":" + String(sample.ledR) + ",";
  json += "\"led_g\":" + String(sample.ledG) + ",";
  json += "\"led_b\":" + String(sample.ledB) + ",";
  json += "\"frame\":" + String(sample.frame) + ",";
  json += "\"last_radar_ms\":" + String(sample.lastRadarMs) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  return json;
}

void applyLedCommand(const String &body) {
  String mode = readJsonString(body, "mode", "solid");
  if (mode == "off") led.mode = LedMode::Off;
  else if (mode == "pulse") led.mode = LedMode::Pulse;
  else if (mode == "rule") led.mode = LedMode::Rule;
  else led.mode = LedMode::Solid;

  led.brightness = constrain(readJsonFloat(body, "brightness", led.brightness), 0.0f, 1.0f);
  led.r = constrain(readJsonInt(body, "r", led.r), 0, 255);
  led.g = constrain(readJsonInt(body, "g", led.g), 0, 255);
  led.b = constrain(readJsonInt(body, "b", led.b), 0, 255);
  led.metric = readJsonString(body, "metric", led.metric);
  led.low = readJsonFloat(body, "low", led.low);
  led.high = readJsonFloat(body, "high", led.high);
  led.lowR = constrain(readJsonInt(body, "low_r", led.lowR), 0, 255);
  led.lowG = constrain(readJsonInt(body, "low_g", led.lowG), 0, 255);
  led.lowB = constrain(readJsonInt(body, "low_b", led.lowB), 0, 255);
  led.okR = constrain(readJsonInt(body, "ok_r", led.okR), 0, 255);
  led.okG = constrain(readJsonInt(body, "ok_g", led.okG), 0, 255);
  led.okB = constrain(readJsonInt(body, "ok_b", led.okB), 0, 255);
  led.highR = constrain(readJsonInt(body, "high_r", led.highR), 0, 255);
  led.highG = constrain(readJsonInt(body, "high_g", led.highG), 0, 255);
  led.highB = constrain(readJsonInt(body, "high_b", led.highB), 0, 255);
  if (led.high < led.low) {
    float swap = led.low;
    led.low = led.high;
    led.high = swap;
  }
  updateLed();
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void setupServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    File file = LittleFS.open(UI_INDEX_PATH, "r");
    if (!file) {
      server.send(500, "text/plain", "Missing /index.html. Run `pio run --target uploadfs`.");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });
  server.serveStatic("/index.html", LittleFS, UI_INDEX_PATH);
  server.on("/api/sample", HTTP_GET, []() {
    sendCors();
    server.send(200, "application/json", sampleJson());
  });
  server.on("/api/led", HTTP_POST, []() {
    applyLedCommand(server.arg("plain"));
    sendCors();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/sample", HTTP_OPTIONS, []() { sendCors(); server.send(204); });
  server.on("/api/led", HTTP_OPTIONS, []() { sendCors(); server.send(204); });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}

void setupOta() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA progress: %u%%\r", (progress * 100) / total);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("auth failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("begin failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("connect failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("receive failed");
    else if (error == OTA_END_ERROR) Serial.println("end failed");
    else Serial.println("unknown");
  });
  ArduinoOTA.begin();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  mmWaveSerial.begin(115200, SERIAL_8N1, MMWAVE_RX_PIN, MMWAVE_TX_PIN);
  mmWave.begin(&mmWaveSerial);
  refreshFirmwareInfo(true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  lightSensorReady = beginBh1750();

  rgbLedWrite(RGB_LED_PIN, 0, 0, 0);
  updateLed();

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed");
  }

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  setupServer();
  setupOta();

  Serial.println();
  Serial.println("MR60BHA2 console ready");
  Serial.print("WiFi: ");
  Serial.println(WIFI_AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("OTA hostname: ");
  Serial.println(OTA_HOSTNAME);
  Serial.print("OTA password: ");
  Serial.println(OTA_PASSWORD);
  Serial.print("BH1750: ");
  Serial.println(lightSensorReady ? "ready" : "not found");
}

void loop() {
  updateRadar();
  updateDerivedPresence();
  updateLight();
  updateLed();
  server.handleClient();
  ArduinoOTA.handle();

  if (millis() - lastSerialReportMs >= 1000) {
    lastSerialReportMs = millis();
    Serial.println(sampleJson());
  }
}
