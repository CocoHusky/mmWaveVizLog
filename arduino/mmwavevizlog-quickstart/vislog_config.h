/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_CONFIG_H
#define VISLOG_CONFIG_H

#include <Arduino.h>
#include "Seeed_Arduino_mmWave.h"

#if __has_include("vislog_private_config.h")
#include "vislog_private_config.h"
#endif

#ifndef VISLOG_WIFI_AP_PASSWORD
#define VISLOG_WIFI_AP_PASSWORD "wirelessphysiology"
#endif

#ifndef VISLOG_OTA_PASSWORD
#define VISLOG_OTA_PASSWORD "wp-ota"
#endif

static constexpr int RADAR_RX_PIN = 17;
static constexpr int RADAR_TX_PIN = 16;
static constexpr int STATUS_LED_PIN = 1;
static constexpr int I2C_SDA_PIN = 22;
static constexpr int I2C_SCL_PIN = 23;
static constexpr uint8_t AMBIENT_LIGHT_I2C_ADDRESS = 0x23;
static constexpr float MAX_VALID_RADAR_RANGE_M = 6.0f;
static constexpr uint8_t MAX_TRACKED_TARGETS = MAX_TARGET_NUM;
// Non-blocking radar reads keep the firmware sample loop responsive.
// Increase to 1-10 ms only if the radar parser starts missing frames.
static constexpr uint8_t RADAR_UPDATE_TIMEOUT_MS = 0;
static constexpr uint16_t STATUS_LED_UPDATE_INTERVAL_MS = 33;
static constexpr uint16_t OTA_IDLE_POLL_INTERVAL_MS = 25;
static constexpr bool SERIAL_JSON_REPORTS = false;
static constexpr bool OTA_PAUSES_SENSOR_COLLECTION = true;

static const char *DEVICE_NAME_PREFIX = "mmWaveVisLog-MR60BHA2";
static const char *OTA_HOSTNAME_PREFIX = "mmWaveVisLog-MR60BHA2-OTA";
static const char *WIFI_AP_PASSWORD = VISLOG_WIFI_AP_PASSWORD;
static const char *OTA_PASSWORD = VISLOG_OTA_PASSWORD;
static const char *VISLOG_FW_VERSION = "0.2.0";

#endif
