/* SPDX-License-Identifier: MIT */

#include "vislog_globals.h"
#include "dashboard_page.h"
#include "json_stream.h"
#include "led_control.h"
#include "web_server.h"

static void sendCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
  server.sendHeader("Cache-Control", "no-store");
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.send_P(200, "text/html", DASHBOARD_PAGE_HTML);
  });
  server.on("/api/sample", HTTP_GET, []() {
    sendCorsHeaders();
    server.send(200, "application/json", buildSensorStateJson());
  });
  server.on("/api/led", HTTP_POST, []() {
    handleLedCommand(server.arg("plain"));
    sendCorsHeaders();
    server.send(200, "application/json", "{\"ok\":true}");
  });
  server.on("/api/sample", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
  server.on("/api/led", HTTP_OPTIONS, []() { sendCorsHeaders(); server.send(204); });
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
}
