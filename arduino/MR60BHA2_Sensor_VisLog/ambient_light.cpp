/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "ambient_light.h"

#include <Wire.h>

bool beginAmbientLightSensor() {
  Wire.beginTransmission(AMBIENT_LIGHT_I2C_ADDRESS);
  Wire.write(0x01);
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(AMBIENT_LIGHT_I2C_ADDRESS);
  Wire.write(0x10);
  return Wire.endTransmission() == 0;
}

void pollAmbientLight() {
  if (!ambientLightReady || millis() - lastAmbientLightReadMs < 250) return;
  lastAmbientLightReadMs = millis();
  if (Wire.requestFrom(AMBIENT_LIGHT_I2C_ADDRESS, uint8_t(2)) != 2) return;
  uint16_t raw = (uint16_t(Wire.read()) << 8) | uint16_t(Wire.read());
  sensorState.light = float(raw) / 1.2f;
}
