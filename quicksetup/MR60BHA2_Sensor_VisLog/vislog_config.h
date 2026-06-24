/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_CONFIG_H
#define VISLOG_CONFIG_H

#include <Arduino.h>
#include "Seeed_Arduino_mmWave.h"

static constexpr int RADAR_RX_PIN = 17;
static constexpr int RADAR_TX_PIN = 16;
static constexpr int STATUS_LED_PIN = 1;
static constexpr int I2C_SDA_PIN = 22;
static constexpr int I2C_SCL_PIN = 23;
static constexpr uint8_t AMBIENT_LIGHT_I2C_ADDRESS = 0x23;
static constexpr float MAX_VALID_RADAR_RANGE_M = 6.0f;
static constexpr uint8_t MAX_TRACKED_TARGETS = MAX_TARGET_NUM;
// 8 ms leaves headroom for web/LED/OTA housekeeping while targeting ~100 Hz.
// Increase to 10-20 ms if the radar parser starts missing frames.
static constexpr uint8_t RADAR_UPDATE_TIMEOUT_MS = 8;
static constexpr uint16_t STATUS_LED_UPDATE_INTERVAL_MS = 33;
static constexpr uint16_t OTA_IDLE_POLL_INTERVAL_MS = 25;
static constexpr bool SERIAL_JSON_REPORTS = false;
static constexpr bool OTA_PAUSES_SENSOR_COLLECTION = true;

static const char *DEVICE_NAME_PREFIX = "mmWaveVisLog-MR60BHA2";
static const char *OTA_HOSTNAME_PREFIX = "mmWaveVisLog-MR60BHA2-OTA";
static const char *WIFI_AP_PASSWORD = "wirelessphysiology";
static const char *OTA_PASSWORD = "wp-ota";
static const char *VISLOG_FW_VERSION = "2.1.4";

#endif
