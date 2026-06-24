/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "radar_sensor.h"

static void resetRadarTargets() {
  sensorState.targetCount = 0;
  sensorState.targetValid = false;
  sensorState.targetX = NAN;
  sensorState.targetY = NAN;
  sensorState.targetDistance = NAN;
  sensorState.targetAngle = NAN;
  sensorState.targetSpeed = NAN;
  for (uint8_t i = 0; i < MAX_TRACKED_TARGETS; i++) {
    sensorState.targets[i] = RadarTarget();
  }
}

static float normalizeRadarCoordinate(float raw);

static void storeRadarTargets(const PeopleCounting &targets, const char *source) {
  resetRadarTargets();
  sensorState.targetSource = source;
  size_t count = targets.targets.size();
  if (count > MAX_TRACKED_TARGETS) count = MAX_TRACKED_TARGETS;
  sensorState.targetCount = uint8_t(count);
  sensorState.targetValid = sensorState.targetCount > 0;
  for (uint8_t i = 0; i < sensorState.targetCount; i++) {
    const TargetN &target = targets.targets[i];
    RadarTarget &out = sensorState.targets[i];
    out.x = normalizeRadarCoordinate(target.x_point);
    out.y = normalizeRadarCoordinate(target.y_point);
    out.distance = hypotf(out.x, out.y);
    out.angle = atan2f(out.x, out.y) * 180.0f / PI;
    out.speed = float(target.dop_index) * RANGE_STEP / 100.0f;
    out.dopIndex = target.dop_index;
    out.clusterIndex = target.cluster_index;
  }
  if (sensorState.targetValid) {
    sensorState.targetX = sensorState.targets[0].x;
    sensorState.targetY = sensorState.targets[0].y;
    sensorState.targetDistance = sensorState.targets[0].distance;
    sensorState.targetAngle = sensorState.targets[0].angle;
    sensorState.targetSpeed = sensorState.targets[0].speed;
    sensorState.lastDetectionSignalMs = millis();
  }
}

static float normalizeRadarRange(float raw) {
  if (!isfinite(raw) || raw <= 0.0f) return NAN;
  float metres = raw > MAX_VALID_RADAR_RANGE_M ? raw / 100.0f : raw;
  return metres >= 0.05f && metres <= MAX_VALID_RADAR_RANGE_M ? metres : NAN;
}

static float normalizeRadarCoordinate(float raw) {
  if (!isfinite(raw)) return NAN;
  return fabsf(raw) > 10.0f ? raw / 100.0f : raw;
}

void refreshPresenceState() {
  uint32_t now = millis();
  if (sensorState.lastTargetMs && now - sensorState.lastTargetMs > 1500) {
    resetRadarTargets();
    sensorState.targetSource = "stale";
  }
  if (sensorState.lastPresenceMs && now - sensorState.lastPresenceMs <= 1200) {
    sensorState.presenceValid = true;
  } else {
    sensorState.presence = false;
    sensorState.presenceValid = false;
  }
  sensorState.presenceSource = sensorState.presenceValid ? "sensor" : "waiting";
}

bool refreshRadarFirmwareInfo(bool force) {
  uint32_t now = millis();
  if (!force && sensorState.firmwareValid && now - sensorState.lastFirmwareMs < 5000) return false;
  FirmwareInfo firmware;
  if (radar.readFirmwareInfo(firmware) != seeed::mmwave::Status::Ok) return false;
  sensorState.firmwareRaw = firmware.value;
  sensorState.firmwareProject = firmware.firmware_verson.project_name;
  sensorState.firmwareMajor = firmware.firmware_verson.major_version;
  sensorState.firmwareSub = firmware.firmware_verson.sub_version;
  sensorState.firmwareModified = firmware.firmware_verson.modified_version;
  sensorState.firmwareValid = true;
  sensorState.lastFirmwareMs = now;
  return true;
}

void pollRadarSensor() {
  if (!radar.update(RADAR_UPDATE_TIMEOUT_MS)) return;
  bool changed = false;

  float total, breath, heart;
  if (radar.getHeartBreathPhases(total, breath, heart)) {
    sensorState.totalPhase = total;
    sensorState.breathPhase = breath;
    sensorState.heartPhase = heart;
    changed = true;
  }

  float value;
  if (radar.getBreathRate(value)) {
    sensorState.breathRate = value > 0.0f && value < 80.0f ? value : NAN;
    if (isfinite(sensorState.breathRate)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (radar.getHeartRate(value)) {
    sensorState.heartRate = value > 0.0f && value < 240.0f ? value : NAN;
    if (isfinite(sensorState.heartRate)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }
  if (radar.getDistance(value)) {
    sensorState.rawDistance = value;
    sensorState.distance = normalizeRadarRange(value);
    if (isfinite(sensorState.distance)) sensorState.lastDetectionSignalMs = millis();
    changed = true;
  }

  bool detected;
  if (radar.readPresence(detected) == seeed::mmwave::Status::Ok) {
    sensorState.presence = detected;
    sensorState.presenceValid = true;
    sensorState.lastPresenceMs = millis();
    sensorState.presenceSource = "sensor";
    changed = true;
  }

  PeopleCounting targets;
  bool gotTargetInfo = false;
  if (radar.readTargetInfo(targets) == seeed::mmwave::Status::Ok) {
    gotTargetInfo = true;
    sensorState.lastTargetMs = millis();
    sensorState.targetInfoValid = true;
    storeRadarTargets(targets, "target info");
    changed = true;
  }
  if (radar.readPointCloud(targets) == seeed::mmwave::Status::Ok) {
    sensorState.lastTargetMs = millis();
    sensorState.pointCloudValid = true;
    if (!gotTargetInfo) {
      storeRadarTargets(targets, "point cloud");
    }
    changed = true;
  }

  if (refreshRadarFirmwareInfo()) changed = true;

  refreshPresenceState();

  if (changed) {
    sensorState.frame++;
    sensorState.lastRadarMs = millis();
  }
}
