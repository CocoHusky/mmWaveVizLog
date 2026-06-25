/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_LED_CONTROL_H
#define VISLOG_LED_CONTROL_H

#include <Arduino.h>

void updateStatusLed();
void playBootRainbow();
void handleLedCommand(const String &body);

#endif
