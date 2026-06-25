/* SPDX-License-Identifier: MIT */

#include "vislog_config.h"
#include "vislog_globals.h"
#include "ambient_light.h"
#include "json_stream.h"
#include "led_control.h"
#include "ota_updates.h"
#include "radar_sensor.h"
#include "web_server.h"

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <Wire.h>
#include "esp32-hal-rgb-led.h"

static void configureDeviceIdentity() {
  uint32_t suffix = uint32_t(ESP.getEfuseMac() & 0xffffff);
  snprintf(deviceName, sizeof(deviceName), "%s-%06lX", DEVICE_NAME_PREFIX, (unsigned long)suffix);
  snprintf(wifiApSsid, sizeof(wifiApSsid), "%s-%06lX", DEVICE_NAME_PREFIX, (unsigned long)suffix);
  snprintf(otaHostname, sizeof(otaHostname), "%s-%06lX", OTA_HOSTNAME_PREFIX, (unsigned long)suffix);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  configureDeviceIdentity();

  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  radar.begin(&radarSerial);
  refreshRadarFirmwareInfo(true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ambientLightReady = beginAmbientLightSensor();

  rgbLedWrite(STATUS_LED_PIN, 0, 0, 0);
  playBootRainbow();
  updateStatusLed();

  WiFi.mode(WIFI_AP);
  WiFi.setSleep(false);
  IPAddress apIP(192, 168, 4, 1);
  IPAddress apGateway(192, 168, 4, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  WiFi.softAPConfig(apIP, apGateway, apSubnet);
  WiFi.softAPsetHostname(wifiApSsid);
  bool apStarted = WiFi.softAP(wifiApSsid, WIFI_AP_PASSWORD);
  setupWebServer();
  setupOtaUpdates();

  Serial.println();
  Serial.println("MR60BHA2 Sensor VisLog ready");
  Serial.print("Device: ");
  Serial.println(deviceName);
  Serial.print("WiFi: ");
  Serial.println(wifiApSsid);
  Serial.print("WiFi AP: ");
  Serial.println(apStarted ? "started" : "failed");
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("OTA hostname: ");
  Serial.println(otaHostname);
  Serial.println("OTA password: configured but not printed");
  Serial.print("BH1750: ");
  Serial.println(ambientLightReady ? "ready" : "not found");
}

void loop() {
  static uint32_t lastStatusLedUpdateMs = 0;

  if (OTA_PAUSES_SENSOR_COLLECTION && otaInProgress) {
    serviceOtaUpdates();
    delay(1);
    return;
  }

  uint32_t now = millis();
  pollRadarSensor();
  refreshPresenceState();
  pollAmbientLight();
  if (now - lastStatusLedUpdateMs >= STATUS_LED_UPDATE_INTERVAL_MS) {
    lastStatusLedUpdateMs = now;
    updateStatusLed();
  }
  server.handleClient();
  serviceOtaUpdates();

  if (SERIAL_JSON_REPORTS && millis() - lastSerialReportMs >= 1000) {
    lastSerialReportMs = millis();
    Serial.println(buildSensorStateJson());
  }
}
