/* SPDX-License-Identifier: MIT */

#include "app_state.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#define VISLOG_FW_VERSION "2.1.4-zephyr"

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

void vislog_app_state_update_simulated(void)
{
	k_mutex_lock(&latest_lock, K_FOREVER);

	latest.frame++;
	latest.uptime_ms = (uint32_t)k_uptime_get();
	latest.last_radar_ms = latest.uptime_ms;

	const uint32_t phase = latest.frame % 40U;
	const float sweep = (float)phase / 40.0f;

	latest.heart_rate = 72.0f + (sweep * 3.0f);
	latest.breath_rate = 16.0f + (sweep * 1.5f);
	latest.distance = 0.70f + (sweep * 0.08f);
	latest.raw_distance = latest.distance;
	latest.total_phase = 0.10f + (sweep * 0.04f);
	latest.breath_phase = 0.05f + (sweep * 0.02f);
	latest.heart_phase = 0.02f + (sweep * 0.01f);

	latest.presence = true;
	latest.presence_valid = true;
	copy_string(latest.presence_source, sizeof(latest.presence_source), "sim");

	latest.target_valid = true;
	copy_string(latest.target_source, sizeof(latest.target_source), "sim");
	latest.target_count = 1;
	latest.target_x = -0.08f + (sweep * 0.03f);
	latest.target_y = latest.distance;
	latest.target_distance = latest.distance;
	latest.target_angle = -6.3f + (sweep * 1.8f);
	latest.target_speed = 0.0f;

	latest.targets[0].valid = true;
	latest.targets[0].x = latest.target_x;
	latest.targets[0].y = latest.target_y;
	latest.targets[0].distance = latest.target_distance;
	latest.targets[0].angle = latest.target_angle;
	latest.targets[0].speed = latest.target_speed;
	latest.targets[0].dop_index = 0;
	latest.targets[0].cluster_index = 0;

	for (size_t i = 1; i < ARRAY_SIZE(latest.targets); i++) {
		latest.targets[i].valid = false;
	}

	latest.light = 120.0f + (sweep * 8.0f);
	latest.light_ready = true;
	latest.led_r = 14;
	latest.led_g = 66;
	latest.led_b = 35;

	k_mutex_unlock(&latest_lock);
}

void vislog_app_state_copy(struct vislog_sample *out)
{
	k_mutex_lock(&latest_lock, K_FOREVER);
	*out = latest;
	k_mutex_unlock(&latest_lock);
}
