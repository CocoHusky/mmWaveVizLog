/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_RADAR_SENSOR_H
#define VISLOG_RADAR_SENSOR_H

void pollRadarSensor();
bool refreshRadarFirmwareInfo(bool force = false);
void refreshPresenceState();

#endif
