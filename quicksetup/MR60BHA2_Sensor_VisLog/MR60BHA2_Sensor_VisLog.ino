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

void setup() {
  Serial.begin(115200);
  delay(300);

  radarSerial.begin(115200, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  radar.begin(&radarSerial);
  refreshRadarFirmwareInfo(true);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  ambientLightReady = beginAmbientLightSensor();

  rgbLedWrite(STATUS_LED_PIN, 0, 0, 0);
  updateStatusLed();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD);
  setupWebServer();
  setupOtaUpdates();

  Serial.println();
  Serial.println("MR60BHA2 Sensor VisLog ready");
  Serial.print("WiFi: ");
  Serial.println(WIFI_AP_SSID);
  Serial.print("Open: http://");
  Serial.println(WiFi.softAPIP());
  Serial.print("OTA hostname: ");
  Serial.println(OTA_HOSTNAME);
  Serial.print("OTA password: ");
  Serial.println(OTA_PASSWORD);
  Serial.print("BH1750: ");
  Serial.println(ambientLightReady ? "ready" : "not found");
}

void loop() {
  pollRadarSensor();
  refreshPresenceState();
  pollAmbientLight();
  updateStatusLed();
  server.handleClient();
  ArduinoOTA.handle();

  if (millis() - lastSerialReportMs >= 1000) {
    lastSerialReportMs = millis();
    Serial.println(buildSensorStateJson());
  }
}
