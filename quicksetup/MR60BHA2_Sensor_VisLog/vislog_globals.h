/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_GLOBALS_H
#define VISLOG_GLOBALS_H

#include <HardwareSerial.h>
#include <WebServer.h>

#include "vislog_types.h"

extern HardwareSerial radarSerial;
extern SEEED_MR60BHA2 radar;
extern WebServer server;
extern SensorSnapshot sensorState;
extern StatusLedConfig statusLed;
extern char deviceName[40];
extern char wifiApSsid[40];
extern char otaHostname[48];
extern bool ambientLightReady;
extern bool otaInProgress;
extern uint32_t lastAmbientLightReadMs;
extern uint32_t lastSerialReportMs;

#endif
