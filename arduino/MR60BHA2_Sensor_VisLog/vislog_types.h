/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_TYPES_H
#define VISLOG_TYPES_H

#include <Arduino.h>
#include "vislog_config.h"

struct RadarTarget {
  float x = NAN;
  float y = NAN;
  float distance = NAN;
  float angle = NAN;
  float speed = NAN;
  int32_t dopIndex = 0;
  int32_t clusterIndex = 0;
};

struct SensorSnapshot {
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
  RadarTarget targets[MAX_TRACKED_TARGETS];
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
  uint32_t radarFrame = 0;
  uint32_t lastRadarMs = 0;
  uint32_t lastPresenceMs = 0;
  uint32_t lastTargetMs = 0;
  uint32_t lastDetectionSignalMs = 0;
  uint32_t lastFirmwareMs = 0;
};

enum class LedMode : uint8_t { Off, Solid, Pulse, Rule };

struct StatusLedConfig {
  LedMode mode = LedMode::Rule;
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

#endif
