/* SPDX-License-Identifier: MIT */

#include "json_stream.h"

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <zephyr/kernel.h>

struct json_writer {
	char *cursor;
	size_t remaining;
};

static int append(struct json_writer *writer, const char *fmt, ...)
{
	va_list args;

	if (writer->remaining == 0) {
		return -EMSGSIZE;
	}

	va_start(args, fmt);
	const int written = vsnprintk(writer->cursor, writer->remaining, fmt, args);
	va_end(args);

	if (written < 0 || (size_t)written >= writer->remaining) {
		return -EMSGSIZE;
	}

	writer->cursor += written;
	writer->remaining -= (size_t)written;
	return 0;
}

static int append_float(struct json_writer *writer, float value, int digits)
{
	if (!isfinite(value)) {
		return append(writer, "null");
	}

	return append(writer, "%.*f", digits, (double)value);
}

static int append_target(struct json_writer *writer, const struct vislog_target *target)
{
	int ret;

	ret = append(writer, "{\"x\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(writer, target->x, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(writer, ",\"y\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(writer, target->y, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(writer, ",\"distance\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(writer, target->distance, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(writer, ",\"angle\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(writer, target->angle, 2);
	if (ret < 0) {
		return ret;
	}
	ret = append(writer, ",\"speed\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(writer, target->speed, 3);
	if (ret < 0) {
		return ret;
	}

	return append(writer, ",\"dop_index\":%d,\"cluster_index\":%d}",
		      target->dop_index, target->cluster_index);
}

int vislog_sample_to_json(const struct vislog_sample *sample, char *buf, size_t len)
{
	if (sample == NULL || buf == NULL || len == 0) {
		return -EINVAL;
	}

	struct json_writer writer = {
		.cursor = buf,
		.remaining = len,
	};

	int ret = append(&writer, "{\"type\":\"sample\",\"heart_rate\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->heart_rate, 2);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"breath_rate\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->breath_rate, 2);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"distance\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->distance, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"raw_distance\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->raw_distance, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"total_phase\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->total_phase, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"breath_phase\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->breath_phase, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"heart_phase\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->heart_phase, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer,
		     ",\"presence\":%s,\"presence_valid\":%s,\"presence_source\":\"%s\""
		     ",\"target_valid\":%s,\"target_source\":\"%s\""
		     ",\"people_count\":%u,\"target_count\":%u,\"target_x\":",
		     sample->presence ? "true" : "false",
		     sample->presence_valid ? "true" : "false",
		     sample->presence_source,
		     sample->target_valid ? "true" : "false",
		     sample->target_source,
		     sample->target_count,
		     sample->target_count);
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->target_x, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"target_y\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->target_y, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"target_distance\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->target_distance, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"target_angle\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->target_angle, 2);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"target_speed\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->target_speed, 3);
	if (ret < 0) {
		return ret;
	}
	ret = append(&writer, ",\"targets\":[");
	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0; i < sample->target_count && i < VISLOG_MAX_TARGETS; i++) {
		if (i > 0) {
			ret = append(&writer, ",");
			if (ret < 0) {
				return ret;
			}
		}
		ret = append_target(&writer, &sample->targets[i]);
		if (ret < 0) {
			return ret;
		}
	}

	ret = append(&writer, "],\"light\":");
	if (ret < 0) {
		return ret;
	}
	ret = append_float(&writer, sample->light, 1);
	if (ret < 0) {
		return ret;
	}

	return append(&writer,
		      ",\"light_ready\":%s,\"console_fw\":\"%s\",\"firmware_valid\":%s"
		      ",\"firmware_raw\":%u,\"firmware_project\":%u,\"firmware_major\":%u"
		      ",\"firmware_sub\":%u,\"firmware_modified\":%u"
		      ",\"led_r\":%u,\"led_g\":%u,\"led_b\":%u"
		      ",\"frame\":%u,\"last_radar_ms\":%u,\"uptime_ms\":%u}",
		      sample->light_ready ? "true" : "false",
		      sample->console_fw,
		      sample->firmware_valid ? "true" : "false",
		      sample->firmware_raw,
		      sample->firmware_project,
		      sample->firmware_major,
		      sample->firmware_sub,
		      sample->firmware_modified,
		      sample->led_r,
		      sample->led_g,
		      sample->led_b,
		      sample->frame,
		      sample->last_radar_ms,
		      sample->uptime_ms);
}

void vislog_print_sample_json(const struct vislog_sample *sample)
{
	char line[VISLOG_JSON_LINE_MAX];

	const int ret = vislog_sample_to_json(sample, line, sizeof(line));
	if (ret == 0) {
		printk("%s\n", line);
	} else {
		printk("{\"type\":\"error\",\"source\":\"json_stream\",\"code\":%d}\n", ret);
	}
}
