/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "ota_updates.h"

#include <ArduinoOTA.h>

void serviceOtaUpdates() {
  static uint32_t lastIdlePollMs = 0;

  if (otaInProgress) {
    ArduinoOTA.handle();
    return;
  }

  uint32_t now = millis();
  if (now - lastIdlePollMs >= OTA_IDLE_POLL_INTERVAL_MS) {
    lastIdlePollMs = now;
    ArduinoOTA.handle();
  }
}

void setupOtaUpdates() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.setRebootOnSuccess(true);
  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    Serial.println("OTA start");
  });
  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    Serial.println("\nOTA complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    static uint8_t lastPercent = 255;
    uint8_t percent = uint8_t((progress * 100) / total);
    if (percent != lastPercent && percent % 10 == 0) {
      lastPercent = percent;
      Serial.printf("OTA progress: %u%%\n", percent);
    }
  });
  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
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
