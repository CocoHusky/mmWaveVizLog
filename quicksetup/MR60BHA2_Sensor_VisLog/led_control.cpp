/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "led_control.h"

static float getMetricValue(const String &metric) {
  if (metric == "heart_rate") return sensorState.heartRate;
  if (metric == "breath_rate") return sensorState.breathRate;
  if (metric == "distance") return sensorState.distance;
  if (metric == "raw_distance") return sensorState.rawDistance;
  if (metric == "presence") return sensorState.presenceValid ? (sensorState.presence ? 1.0f : 0.0f) : NAN;
  if (metric == "people_count") return sensorState.targetInfoValid || sensorState.pointCloudValid ? float(sensorState.targetCount) : NAN;
  if (metric == "target_x") return sensorState.targetX;
  if (metric == "target_y") return sensorState.targetY;
  if (metric == "target_angle") return sensorState.targetAngle;
  if (metric == "target_speed") return sensorState.targetSpeed;
  if (metric == "light") return sensorState.light;
  if (metric == "total_phase") return sensorState.totalPhase;
  if (metric == "breath_phase") return sensorState.breathPhase;
  if (metric == "heart_phase") return sensorState.heartPhase;
  return NAN;
}

static void writeStatusLed(uint8_t r, uint8_t g, uint8_t b, float brightness) {
  brightness = constrain(brightness, 0.0f, 1.0f);
  sensorState.ledR = uint8_t(r * brightness);
  sensorState.ledG = uint8_t(g * brightness);
  sensorState.ledB = uint8_t(b * brightness);
  rgbLedWrite(STATUS_LED_PIN, sensorState.ledR, sensorState.ledG, sensorState.ledB);
}

void updateStatusLed() {
  if (statusLed.mode == LedMode::Off) {
    writeStatusLed(0, 0, 0, 1.0f);
    return;
  }
  if (statusLed.mode == LedMode::Pulse) {
    float wave = (sinf(float(millis()) * 0.004f) + 1.0f) * 0.5f;
    writeStatusLed(statusLed.r, statusLed.g, statusLed.b,
                   statusLed.brightness * (0.12f + 0.88f * wave));
    return;
  }
  if (statusLed.mode == LedMode::Rule) {
    float value = getMetricValue(statusLed.metric);
    if (!isfinite(value)) writeStatusLed(20, 20, 20, statusLed.brightness);
    else if (value < statusLed.low) writeStatusLed(statusLed.lowR, statusLed.lowG, statusLed.lowB, statusLed.brightness);
    else if (value > statusLed.high) writeStatusLed(statusLed.highR, statusLed.highG, statusLed.highB, statusLed.brightness);
    else writeStatusLed(statusLed.okR, statusLed.okG, statusLed.okB, statusLed.brightness);
    return;
  }

  writeStatusLed(statusLed.r, statusLed.g, statusLed.b, statusLed.brightness);
}

static String readJsonTextField(const String &body, const char *key, const String &fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  int start = body.indexOf('"', colonPos + 1);
  int end = body.indexOf('"', start + 1);
  return start >= 0 && end > start ? body.substring(start + 1, end) : fallback;
}

static float readJsonFloatField(const String &body, const char *key, float fallback) {
  String needle = "\"" + String(key) + "\"";
  int keyPos = body.indexOf(needle);
  if (keyPos < 0) return fallback;
  int colonPos = body.indexOf(':', keyPos);
  if (colonPos < 0) return fallback;
  return body.substring(colonPos + 1).toFloat();
}

static int readJsonIntField(const String &body, const char *key, int fallback) {
  return int(readJsonFloatField(body, key, float(fallback)));
}

void handleLedCommand(const String &body) {
  String mode = readJsonTextField(body, "mode", "solid");
  if (mode == "off") statusLed.mode = LedMode::Off;
  else if (mode == "pulse") statusLed.mode = LedMode::Pulse;
  else if (mode == "rule") statusLed.mode = LedMode::Rule;
  else statusLed.mode = LedMode::Solid;

  statusLed.brightness = constrain(readJsonFloatField(body, "brightness", statusLed.brightness), 0.0f, 1.0f);
  statusLed.r = constrain(readJsonIntField(body, "r", statusLed.r), 0, 255);
  statusLed.g = constrain(readJsonIntField(body, "g", statusLed.g), 0, 255);
  statusLed.b = constrain(readJsonIntField(body, "b", statusLed.b), 0, 255);
  statusLed.metric = readJsonTextField(body, "metric", statusLed.metric);
  statusLed.low = readJsonFloatField(body, "low", statusLed.low);
  statusLed.high = readJsonFloatField(body, "high", statusLed.high);
  statusLed.lowR = constrain(readJsonIntField(body, "low_r", statusLed.lowR), 0, 255);
  statusLed.lowG = constrain(readJsonIntField(body, "low_g", statusLed.lowG), 0, 255);
  statusLed.lowB = constrain(readJsonIntField(body, "low_b", statusLed.lowB), 0, 255);
  statusLed.okR = constrain(readJsonIntField(body, "ok_r", statusLed.okR), 0, 255);
  statusLed.okG = constrain(readJsonIntField(body, "ok_g", statusLed.okG), 0, 255);
  statusLed.okB = constrain(readJsonIntField(body, "ok_b", statusLed.okB), 0, 255);
  statusLed.highR = constrain(readJsonIntField(body, "high_r", statusLed.highR), 0, 255);
  statusLed.highG = constrain(readJsonIntField(body, "high_g", statusLed.highG), 0, 255);
  statusLed.highB = constrain(readJsonIntField(body, "high_b", statusLed.highB), 0, 255);
  if (statusLed.high < statusLed.low) {
    float swap = statusLed.low;
    statusLed.low = statusLed.high;
    statusLed.high = swap;
  }
  updateStatusLed();
}
