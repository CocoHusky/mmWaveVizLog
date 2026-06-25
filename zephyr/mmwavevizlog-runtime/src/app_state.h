/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_APP_STATE_H
#define VISLOG_APP_STATE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../drivers/mr60bha2/mr60bha2.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VISLOG_MAX_TARGETS 3
#define VISLOG_SOURCE_LEN 16
#define VISLOG_FW_LEN 20

struct vislog_target {
	bool valid;
	float x;
	float y;
	float distance;
	float angle;
	float speed;
	int32_t dop_index;
	int32_t cluster_index;
};

struct vislog_sample {
	float heart_rate;
	float breath_rate;
	float distance;
	float raw_distance;
	float total_phase;
	float breath_phase;
	float heart_phase;
	bool presence;
	bool presence_valid;
	char presence_source[VISLOG_SOURCE_LEN];
	bool target_valid;
	char target_source[VISLOG_SOURCE_LEN];
	uint8_t target_count;
	float target_x;
	float target_y;
	float target_distance;
	float target_angle;
	float target_speed;
	struct vislog_target targets[VISLOG_MAX_TARGETS];
	float light;
	bool light_ready;
	char console_fw[VISLOG_FW_LEN];
	bool firmware_valid;
	uint32_t firmware_raw;
	uint8_t firmware_project;
	uint8_t firmware_major;
	uint8_t firmware_sub;
	uint8_t firmware_modified;
	uint8_t led_r;
	uint8_t led_g;
	uint8_t led_b;
	uint32_t frame;
	uint32_t last_radar_ms;
	uint32_t last_presence_ms;
	uint32_t last_target_ms;
	uint32_t last_firmware_ms;
	uint32_t uptime_ms;
};

void vislog_app_state_init(void);
void vislog_app_state_apply_radar_snapshot(const struct mr60bha2_snapshot *snapshot);
void vislog_app_state_copy(struct vislog_sample *out);
void vislog_app_state_set_light(float light_lux, bool ready);
void vislog_app_state_set_led_rgb(uint8_t r, uint8_t g, uint8_t b);

#ifdef __cplusplus
}
#endif

#endif
