/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"

HardwareSerial radarSerial(1);
SEEED_MR60BHA2 radar;
WebServer server(80);
SensorSnapshot sensorState;
StatusLedConfig statusLed;
char deviceName[40] = "mmWaveVisLog-MR60BHA2";
char wifiApSsid[40] = "mmWaveVisLog-MR60BHA2";
char otaHostname[48] = "mmWaveVisLog-MR60BHA2-OTA";
bool ambientLightReady = false;
bool otaInProgress = false;
uint32_t lastAmbientLightReadMs = 0;
uint32_t lastSerialReportMs = 0;
