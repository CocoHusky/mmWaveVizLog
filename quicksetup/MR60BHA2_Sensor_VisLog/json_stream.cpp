/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "json_stream.h"

static String jsonNumber(float value, unsigned int digits) {
  if (!isfinite(value)) return "null";
  return String(value, digits);
}

String buildSensorStateJson() {
  sensorState.frame++;

  String json;
  json.reserve(1180);
  json = "{";
  json += "\"heart_rate\":" + jsonNumber(sensorState.heartRate, 2) + ",";
  json += "\"breath_rate\":" + jsonNumber(sensorState.breathRate, 2) + ",";
  json += "\"distance\":" + jsonNumber(sensorState.distance, 3) + ",";
  json += "\"raw_distance\":" + jsonNumber(sensorState.rawDistance, 3) + ",";
  json += "\"total_phase\":" + jsonNumber(sensorState.totalPhase, 3) + ",";
  json += "\"breath_phase\":" + jsonNumber(sensorState.breathPhase, 3) + ",";
  json += "\"heart_phase\":" + jsonNumber(sensorState.heartPhase, 3) + ",";
  json += "\"presence\":" + String(sensorState.presence ? "true" : "false") + ",";
  json += "\"presence_valid\":" + String(sensorState.presenceValid ? "true" : "false") + ",";
  json += "\"presence_source\":\"" + sensorState.presenceSource + "\",";
  json += "\"target_valid\":" + String(sensorState.targetValid ? "true" : "false") + ",";
  json += "\"target_source\":\"" + sensorState.targetSource + "\",";
  json += "\"people_count\":" + String(sensorState.targetCount) + ",";
  json += "\"target_count\":" + String(sensorState.targetCount) + ",";
  json += "\"target_x\":" + jsonNumber(sensorState.targetX, 3) + ",";
  json += "\"target_y\":" + jsonNumber(sensorState.targetY, 3) + ",";
  json += "\"target_distance\":" + jsonNumber(sensorState.targetDistance, 3) + ",";
  json += "\"target_angle\":" + jsonNumber(sensorState.targetAngle, 2) + ",";
  json += "\"target_speed\":" + jsonNumber(sensorState.targetSpeed, 3) + ",";
  json += "\"targets\":[";
  for (uint8_t i = 0; i < sensorState.targetCount; i++) {
    if (i) json += ",";
    json += "{";
    json += "\"x\":" + jsonNumber(sensorState.targets[i].x, 3) + ",";
    json += "\"y\":" + jsonNumber(sensorState.targets[i].y, 3) + ",";
    json += "\"distance\":" + jsonNumber(sensorState.targets[i].distance, 3) + ",";
    json += "\"angle\":" + jsonNumber(sensorState.targets[i].angle, 2) + ",";
    json += "\"speed\":" + jsonNumber(sensorState.targets[i].speed, 3) + ",";
    json += "\"dop_index\":" + String(sensorState.targets[i].dopIndex) + ",";
    json += "\"cluster_index\":" + String(sensorState.targets[i].clusterIndex);
    json += "}";
  }
  json += "],";
  json += "\"light\":" + jsonNumber(sensorState.light, 1) + ",";
  json += "\"light_ready\":" + String(ambientLightReady ? "true" : "false") + ",";
  json += "\"console_fw\":\"" + String(VISLOG_FW_VERSION) + "\",";
  json += "\"firmware_valid\":" + String(sensorState.firmwareValid ? "true" : "false") + ",";
  json += "\"firmware_raw\":" + String(sensorState.firmwareRaw) + ",";
  json += "\"firmware_project\":" + String(sensorState.firmwareProject) + ",";
  json += "\"firmware_major\":" + String(sensorState.firmwareMajor) + ",";
  json += "\"firmware_sub\":" + String(sensorState.firmwareSub) + ",";
  json += "\"firmware_modified\":" + String(sensorState.firmwareModified) + ",";
  json += "\"led_r\":" + String(sensorState.ledR) + ",";
  json += "\"led_g\":" + String(sensorState.ledG) + ",";
  json += "\"led_b\":" + String(sensorState.ledB) + ",";
  json += "\"frame\":" + String(sensorState.frame) + ",";
  json += "\"radar_frame\":" + String(sensorState.radarFrame) + ",";
  json += "\"last_radar_ms\":" + String(sensorState.lastRadarMs) + ",";
  json += "\"uptime_ms\":" + String(millis());
  json += "}";
  return json;
}
