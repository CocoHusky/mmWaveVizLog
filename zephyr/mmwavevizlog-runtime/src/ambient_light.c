/* SPDX-License-Identifier: MIT */

#include "ambient_light.h"

#include <math.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app_state.h"

LOG_MODULE_REGISTER(vislog_light, LOG_LEVEL_INF);

#define AMBIENT_LIGHT_UPDATE_INTERVAL_MS 250U

static const struct device *const bh1750 = DEVICE_DT_GET(DT_NODELABEL(bh1750));
static uint32_t last_light_ms;
static bool light_ready;

static float sensor_value_to_lux(const struct sensor_value *value)
{
	if (value == NULL) {
		return NAN;
	}

	return (float)value->val1 + ((float)value->val2 / 1000000.0f);
}

void vislog_ambient_light_init(void)
{
	struct sensor_value value;
	int ret;

	last_light_ms = 0U;
	light_ready = false;
	vislog_app_state_set_light(NAN, false);

	if (!device_is_ready(bh1750)) {
		LOG_WRN("BH1750 device not ready");
		return;
	}

	ret = sensor_sample_fetch(bh1750);
	if (ret != 0) {
		LOG_WRN("BH1750 probe failed: %d", ret);
		return;
	}

	ret = sensor_channel_get(bh1750, SENSOR_CHAN_LIGHT, &value);
	if (ret != 0) {
		LOG_WRN("BH1750 read failed during init: %d", ret);
		return;
	}

	light_ready = true;
	last_light_ms = k_uptime_get_32();
	vislog_app_state_set_light(sensor_value_to_lux(&value), true);
	LOG_INF("BH1750 ready");
}

void vislog_ambient_light_poll(void)
{
	struct sensor_value value;
	const uint32_t now = k_uptime_get_32();
	int ret;

	if (!light_ready) {
		return;
	}

	if ((uint32_t)(now - last_light_ms) < AMBIENT_LIGHT_UPDATE_INTERVAL_MS) {
		return;
	}

	last_light_ms = now;
	ret = sensor_sample_fetch(bh1750);
	if (ret != 0) {
		LOG_WRN("BH1750 sample fetch failed: %d", ret);
		return;
	}

	ret = sensor_channel_get(bh1750, SENSOR_CHAN_LIGHT, &value);
	if (ret != 0) {
		LOG_WRN("BH1750 channel read failed: %d", ret);
		return;
	}

	vislog_app_state_set_light(sensor_value_to_lux(&value), true);
}
