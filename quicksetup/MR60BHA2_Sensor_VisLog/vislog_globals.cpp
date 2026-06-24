/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"

HardwareSerial radarSerial(1);
SEEED_MR60BHA2 radar;
WebServer server(80);
SensorSnapshot sensorState;
StatusLedConfig statusLed;
bool ambientLightReady = false;
bool otaInProgress = false;
uint32_t lastAmbientLightReadMs = 0;
uint32_t lastSerialReportMs = 0;
