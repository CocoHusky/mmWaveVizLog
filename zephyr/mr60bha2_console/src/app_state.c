/* SPDX-License-Identifier: MIT */

#include "app_state.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define VISLOG_FW_VERSION "2.1.4"

static struct vislog_sample latest;
static struct k_mutex latest_lock;

static void copy_string(char *dst, size_t dst_len, const char *src)
{
	if (dst_len == 0) {
		return;
	}

	(void)snprintf(dst, dst_len, "%s", src);
}

static void set_default_sample(struct vislog_sample *sample)
{
	memset(sample, 0, sizeof(*sample));

	sample->heart_rate = NAN;
	sample->breath_rate = NAN;
	sample->distance = NAN;
	sample->raw_distance = NAN;
	sample->total_phase = NAN;
	sample->breath_phase = NAN;
	sample->heart_phase = NAN;
	sample->target_x = NAN;
	sample->target_y = NAN;
	sample->target_distance = NAN;
	sample->target_angle = NAN;
	sample->target_speed = NAN;
	sample->light = NAN;
	sample->last_presence_ms = 0;
	sample->last_target_ms = 0;
	sample->last_firmware_ms = 0;

	for (size_t i = 0; i < ARRAY_SIZE(sample->targets); i++) {
		sample->targets[i].x = NAN;
		sample->targets[i].y = NAN;
		sample->targets[i].distance = NAN;
		sample->targets[i].angle = NAN;
		sample->targets[i].speed = NAN;
	}

	copy_string(sample->presence_source, sizeof(sample->presence_source), "waiting");
	copy_string(sample->target_source, sizeof(sample->target_source), "waiting");
	copy_string(sample->console_fw, sizeof(sample->console_fw), VISLOG_FW_VERSION);
}

void vislog_app_state_init(void)
{
	k_mutex_init(&latest_lock);
	set_default_sample(&latest);
}

static void clear_target_fields(struct vislog_sample *sample)
{
	sample->target_valid = false;
	sample->target_count = 0;
	sample->target_x = NAN;
	sample->target_y = NAN;
	sample->target_distance = NAN;
	sample->target_angle = NAN;
	sample->target_speed = NAN;

	for (size_t i = 0; i < ARRAY_SIZE(sample->targets); i++) {
		sample->targets[i].valid = false;
		sample->targets[i].x = NAN;
		sample->targets[i].y = NAN;
		sample->targets[i].distance = NAN;
		sample->targets[i].angle = NAN;
		sample->targets[i].speed = NAN;
		sample->targets[i].dop_index = 0;
		sample->targets[i].cluster_index = 0;
	}
}

void vislog_app_state_apply_radar_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	k_mutex_lock(&latest_lock, K_FOREVER);

	latest.frame++;
	latest.uptime_ms = (uint32_t)k_uptime_get();
	latest.last_radar_ms = latest.uptime_ms;

	if (snapshot != NULL) {
		if (isfinite(snapshot->heart_rate)) {
			latest.heart_rate = snapshot->heart_rate;
		}
		if (isfinite(snapshot->breath_rate)) {
			latest.breath_rate = snapshot->breath_rate;
		}
		if (isfinite(snapshot->distance)) {
			latest.distance = snapshot->distance;
		}
		if (isfinite(snapshot->raw_distance)) {
			latest.raw_distance = snapshot->raw_distance;
		}
		if (isfinite(snapshot->total_phase)) {
			latest.total_phase = snapshot->total_phase;
		}
		if (isfinite(snapshot->breath_phase)) {
			latest.breath_phase = snapshot->breath_phase;
		}
		if (isfinite(snapshot->heart_phase)) {
			latest.heart_phase = snapshot->heart_phase;
		}
		if (snapshot->presence_valid) {
			latest.presence = snapshot->presence;
			latest.presence_valid = true;
			latest.last_presence_ms = latest.uptime_ms;
			copy_string(latest.presence_source, sizeof(latest.presence_source), snapshot->presence_source);
		}
		if (snapshot->target_valid) {
			latest.target_valid = true;
			latest.last_target_ms = latest.uptime_ms;
			latest.target_count = snapshot->target_count;
			copy_string(latest.target_source, sizeof(latest.target_source), snapshot->target_source);

			for (size_t i = 0; i < ARRAY_SIZE(latest.targets); i++) {
				latest.targets[i].valid = false;
				latest.targets[i].x = NAN;
				latest.targets[i].y = NAN;
				latest.targets[i].distance = NAN;
				latest.targets[i].angle = NAN;
				latest.targets[i].speed = NAN;
				latest.targets[i].dop_index = 0;
				latest.targets[i].cluster_index = 0;
			}

			for (size_t i = 0; i < latest.target_count && i < ARRAY_SIZE(latest.targets); i++) {
				latest.targets[i].valid = snapshot->targets[i].valid;
				latest.targets[i].x = snapshot->targets[i].x;
				latest.targets[i].y = snapshot->targets[i].y;
				latest.targets[i].distance = snapshot->targets[i].distance;
				latest.targets[i].angle = snapshot->targets[i].angle;
				latest.targets[i].speed = snapshot->targets[i].speed;
				latest.targets[i].dop_index = snapshot->targets[i].dop_index;
				latest.targets[i].cluster_index = snapshot->targets[i].cluster_index;
			}
			if (latest.target_count > 0 && latest.targets[0].valid) {
				latest.target_x = latest.targets[0].x;
				latest.target_y = latest.targets[0].y;
				latest.target_distance = latest.targets[0].distance;
				latest.target_angle = latest.targets[0].angle;
				latest.target_speed = latest.targets[0].speed;
			}
		}
		if (snapshot->firmware_valid) {
			latest.firmware_valid = true;
			latest.last_firmware_ms = latest.uptime_ms;
			latest.firmware_raw = snapshot->firmware_raw;
			latest.firmware_project = snapshot->firmware_project;
			latest.firmware_major = snapshot->firmware_major;
			latest.firmware_sub = snapshot->firmware_sub;
			latest.firmware_modified = snapshot->firmware_modified;
		}
	}

	if (latest.presence_valid && latest.last_presence_ms &&
	    latest.uptime_ms - latest.last_presence_ms > 1200U) {
		latest.presence = false;
		latest.presence_valid = false;
		copy_string(latest.presence_source, sizeof(latest.presence_source), "waiting");
	}
	if (latest.target_valid && latest.last_target_ms &&
	    latest.uptime_ms - latest.last_target_ms > 1500U) {
		clear_target_fields(&latest);
		copy_string(latest.target_source, sizeof(latest.target_source), "stale");
	}
	copy_string(latest.console_fw, sizeof(latest.console_fw), VISLOG_FW_VERSION);

	k_mutex_unlock(&latest_lock);
}

void vislog_app_state_copy(struct vislog_sample *out)
{
	k_mutex_lock(&latest_lock, K_FOREVER);
	*out = latest;
	k_mutex_unlock(&latest_lock);
}

void vislog_app_state_set_light(float light_lux, bool ready)
{
	k_mutex_lock(&latest_lock, K_FOREVER);
	latest.light = light_lux;
	latest.light_ready = ready;
	k_mutex_unlock(&latest_lock);
}

void vislog_app_state_set_led_rgb(uint8_t r, uint8_t g, uint8_t b)
{
	k_mutex_lock(&latest_lock, K_FOREVER);
	latest.led_r = r;
	latest.led_g = g;
	latest.led_b = b;
	k_mutex_unlock(&latest_lock);
}
