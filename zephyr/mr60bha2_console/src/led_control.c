/* SPDX-License-Identifier: MIT */

#include "led_control.h"

#include <math.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(vislog_led, LOG_LEVEL_INF);

#define LED_BRIGHTNESS_DEFAULT 0.35f
#define LED_BOOT_FLASH_MS 80

enum vislog_led_mode {
	VISLOG_LED_OFF,
	VISLOG_LED_SOLID,
	VISLOG_LED_PULSE,
	VISLOG_LED_RULE,
};

struct vislog_led_config {
	enum vislog_led_mode mode;
	float brightness;
	uint8_t r;
	uint8_t g;
	uint8_t b;
	char metric[24];
	float low;
	float high;
	uint8_t low_r;
	uint8_t low_g;
	uint8_t low_b;
	uint8_t ok_r;
	uint8_t ok_g;
	uint8_t ok_b;
	uint8_t high_r;
	uint8_t high_g;
	uint8_t high_b;
};

static const struct device *const led_strip = DEVICE_DT_GET(DT_ALIAS(led_strip));
static struct k_mutex led_lock;
static struct vislog_led_config led_config;
static struct vislog_sample led_sample;
static bool led_ready;
static bool led_sample_valid;

static uint8_t scale_channel(uint8_t channel, float brightness)
{
	brightness = fmaxf(0.0f, fminf(brightness, 1.0f));
	float scaled = channel * brightness;
	if (scaled <= 0.0f) {
		return 0U;
	}
	if (scaled >= 255.0f) {
		return 255U;
	}
	return (uint8_t)(scaled + 0.5f);
}

static void led_rgb_apply(uint8_t r, uint8_t g, uint8_t b)
{
	struct led_rgb pixel = {
		.r = r,
		.g = g,
		.b = b,
	};

	vislog_app_state_set_led_rgb(r, g, b);

	if (!led_ready) {
		return;
	}

	struct led_rgb pixels[1] = { pixel };
	int ret = led_strip_update_rgb(led_strip, pixels, 1);
	if (ret != 0) {
		LOG_WRN("LED update failed: %d", ret);
	}
}

static float sample_metric_value(const struct vislog_sample *sample, const char *metric)
{
	if (sample == NULL || metric == NULL) {
		return NAN;
	}

	if (strcmp(metric, "heart_rate") == 0) {
		return sample->heart_rate;
	}
	if (strcmp(metric, "breath_rate") == 0) {
		return sample->breath_rate;
	}
	if (strcmp(metric, "distance") == 0) {
		return sample->distance;
	}
	if (strcmp(metric, "raw_distance") == 0) {
		return sample->raw_distance;
	}
	if (strcmp(metric, "presence") == 0) {
		return sample->presence_valid ? (sample->presence ? 1.0f : 0.0f) : NAN;
	}
	if (strcmp(metric, "people_count") == 0) {
		return sample->target_valid ? (float)sample->target_count : NAN;
	}
	if (strcmp(metric, "target_x") == 0) {
		return sample->target_x;
	}
	if (strcmp(metric, "target_y") == 0) {
		return sample->target_y;
	}
	if (strcmp(metric, "target_angle") == 0) {
		return sample->target_angle;
	}
	if (strcmp(metric, "target_speed") == 0) {
		return sample->target_speed;
	}
	if (strcmp(metric, "light") == 0) {
		return sample->light;
	}
	if (strcmp(metric, "total_phase") == 0) {
		return sample->total_phase;
	}
	if (strcmp(metric, "breath_phase") == 0) {
		return sample->breath_phase;
	}
	if (strcmp(metric, "heart_phase") == 0) {
		return sample->heart_phase;
	}

	return NAN;
}

static void parse_string_field(const char *body, const char *key, char *out, size_t out_len)
{
	const char *match;
	const char *value;
	const char *end;
	size_t len;

	if (body == NULL || key == NULL || out == NULL || out_len == 0) {
		return;
	}

	match = strstr(body, key);
	if (match == NULL) {
		return;
	}

	value = strchr(match, ':');
	if (value == NULL) {
		return;
	}
	value++;
	while (*value == ' ' || *value == '\t' || *value == '\n' || *value == '\r') {
		value++;
	}
	if (*value == '"') {
		value++;
	}
	end = value;
	while (*end != '\0' && *end != '"' && *end != ',' && *end != '}') {
		end++;
	}
	len = (size_t)(end - value);
	if (len >= out_len) {
		len = out_len - 1;
	}
	memcpy(out, value, len);
	out[len] = '\0';
}

static bool parse_float_field(const char *body, const char *key, float *out)
{
	const char *match;
	const char *value;

	if (body == NULL || key == NULL || out == NULL) {
		return false;
	}

	match = strstr(body, key);
	if (match == NULL) {
		return false;
	}

	value = strchr(match, ':');
	if (value == NULL) {
		return false;
	}

	*out = strtof(value + 1, NULL);
	return true;
}

static bool parse_int_field(const char *body, const char *key, int *out)
{
	float value;

	if (!parse_float_field(body, key, &value)) {
		return false;
	}
	*out = (int)value;
	return true;
}

static void render_locked(void)
{
	uint8_t out_r = 0;
	uint8_t out_g = 0;
	uint8_t out_b = 0;
	float brightness = led_config.brightness;
	const struct vislog_sample *sample = led_sample_valid ? &led_sample : NULL;

	switch (led_config.mode) {
	case VISLOG_LED_OFF:
		break;
	case VISLOG_LED_PULSE: {
		float wave = (sinf((float)k_uptime_get_32() * 0.004f) + 1.0f) * 0.5f;
		brightness *= (0.12f + 0.88f * wave);
		out_r = led_config.r;
		out_g = led_config.g;
		out_b = led_config.b;
		break;
	}
	case VISLOG_LED_RULE: {
		float metric = sample_metric_value(sample, led_config.metric);

		if (!isfinite(metric)) {
			out_r = 20;
			out_g = 20;
			out_b = 20;
		} else if (metric < led_config.low) {
			out_r = led_config.low_r;
			out_g = led_config.low_g;
			out_b = led_config.low_b;
		} else if (metric > led_config.high) {
			out_r = led_config.high_r;
			out_g = led_config.high_g;
			out_b = led_config.high_b;
		} else {
			out_r = led_config.ok_r;
			out_g = led_config.ok_g;
			out_b = led_config.ok_b;
		}
		break;
	}
	case VISLOG_LED_SOLID:
	default:
		out_r = led_config.r;
		out_g = led_config.g;
		out_b = led_config.b;
		break;
	}

	led_rgb_apply(scale_channel(out_r, brightness), scale_channel(out_g, brightness),
		      scale_channel(out_b, brightness));
}

void vislog_led_init(void)
{
	k_mutex_init(&led_lock);

	led_config.mode = VISLOG_LED_SOLID;
	led_config.brightness = LED_BRIGHTNESS_DEFAULT;
	led_config.r = 40;
	led_config.g = 190;
	led_config.b = 100;
	strcpy(led_config.metric, "target_speed");
	led_config.low = -0.05f;
	led_config.high = 0.05f;
	led_config.low_r = 255;
	led_config.low_g = 0;
	led_config.low_b = 0;
	led_config.ok_r = 0;
	led_config.ok_g = 0;
	led_config.ok_b = 0;
	led_config.high_r = 0;
	led_config.high_g = 80;
	led_config.high_b = 255;

	led_ready = device_is_ready(led_strip);
	if (!led_ready) {
		LOG_ERR("LED strip device not ready");
		vislog_app_state_set_led_rgb(0, 0, 0);
		return;
	}

	/* Boot indication: render the default solid color immediately. */
	k_mutex_lock(&led_lock, K_FOREVER);
	render_locked();
	k_mutex_unlock(&led_lock);

	k_msleep(LED_BOOT_FLASH_MS);
}

void vislog_led_update_from_sample(const struct vislog_sample *sample)
{
	if (sample == NULL) {
		return;
	}

	k_mutex_lock(&led_lock, K_FOREVER);
	led_sample = *sample;
	led_sample_valid = true;
	render_locked();
	k_mutex_unlock(&led_lock);
}

int vislog_led_apply_json(const char *body)
{
	char mode[16];
	char metric[24];
	int value;

	if (body == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&led_lock, K_FOREVER);

	strcpy(mode, "solid");
	parse_string_field(body, "\"mode\"", mode, sizeof(mode));
	if (strcmp(mode, "off") == 0) {
		led_config.mode = VISLOG_LED_OFF;
	} else if (strcmp(mode, "pulse") == 0) {
		led_config.mode = VISLOG_LED_PULSE;
	} else if (strcmp(mode, "rule") == 0) {
		led_config.mode = VISLOG_LED_RULE;
	} else {
		led_config.mode = VISLOG_LED_SOLID;
	}

	parse_float_field(body, "\"brightness\"", &led_config.brightness);
	led_config.brightness = fmaxf(0.0f, fminf(led_config.brightness, 1.0f));

	value = led_config.r;
	if (parse_int_field(body, "\"r\"", &value)) {
		led_config.r = CLAMP(value, 0, 255);
	}
	value = led_config.g;
	if (parse_int_field(body, "\"g\"", &value)) {
		led_config.g = CLAMP(value, 0, 255);
	}
	value = led_config.b;
	if (parse_int_field(body, "\"b\"", &value)) {
		led_config.b = CLAMP(value, 0, 255);
	}

	strcpy(metric, led_config.metric);
	parse_string_field(body, "\"metric\"", metric, sizeof(metric));
	strncpy(led_config.metric, metric, sizeof(led_config.metric) - 1);
	led_config.metric[sizeof(led_config.metric) - 1] = '\0';

	parse_float_field(body, "\"low\"", &led_config.low);
	parse_float_field(body, "\"high\"", &led_config.high);
	if (led_config.high < led_config.low) {
		float swap = led_config.low;
		led_config.low = led_config.high;
		led_config.high = swap;
	}

	value = led_config.low_r;
	if (parse_int_field(body, "\"low_r\"", &value)) {
		led_config.low_r = CLAMP(value, 0, 255);
	}
	value = led_config.low_g;
	if (parse_int_field(body, "\"low_g\"", &value)) {
		led_config.low_g = CLAMP(value, 0, 255);
	}
	value = led_config.low_b;
	if (parse_int_field(body, "\"low_b\"", &value)) {
		led_config.low_b = CLAMP(value, 0, 255);
	}

	value = led_config.ok_r;
	if (parse_int_field(body, "\"ok_r\"", &value)) {
		led_config.ok_r = CLAMP(value, 0, 255);
	}
	value = led_config.ok_g;
	if (parse_int_field(body, "\"ok_g\"", &value)) {
		led_config.ok_g = CLAMP(value, 0, 255);
	}
	value = led_config.ok_b;
	if (parse_int_field(body, "\"ok_b\"", &value)) {
		led_config.ok_b = CLAMP(value, 0, 255);
	}

	value = led_config.high_r;
	if (parse_int_field(body, "\"high_r\"", &value)) {
		led_config.high_r = CLAMP(value, 0, 255);
	}
	value = led_config.high_g;
	if (parse_int_field(body, "\"high_g\"", &value)) {
		led_config.high_g = CLAMP(value, 0, 255);
	}
	value = led_config.high_b;
	if (parse_int_field(body, "\"high_b\"", &value)) {
		led_config.high_b = CLAMP(value, 0, 255);
	}

	render_locked();
	k_mutex_unlock(&led_lock);

	return 0;
}
