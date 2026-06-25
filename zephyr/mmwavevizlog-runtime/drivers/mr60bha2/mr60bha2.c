/* SPDX-License-Identifier: MIT */

#include "mr60bha2.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define MR60BHA2_FRAME_SOF 0x01
#define MR60BHA2_FRAME_HEADER_LEN 8
#define MR60BHA2_FRAME_CHECKSUM_LEN 1

#define MR60BHA2_TYPE_HEART_BREATH_PHASE 0x0A13
#define MR60BHA2_TYPE_BREATH_RATE 0x0A14
#define MR60BHA2_TYPE_HEART_RATE 0x0A15
#define MR60BHA2_TYPE_DISTANCE 0x0A16
#define MR60BHA2_TYPE_POINT_CLOUD 0x0A08
#define MR60BHA2_TYPE_TARGET_INFO 0x0A04
#define MR60BHA2_TYPE_HUMAN_DETECTION 0x0F09
#define MR60BHA2_TYPE_FIRMWARE 0xFFFF
#define MR60BHA2_PI 3.14159265358979323846f

static float extract_float(const uint8_t *bytes)
{
	float value;

	memcpy(&value, bytes, sizeof(value));
	return value;
}

static uint32_t extract_u32(const uint8_t *bytes)
{
	uint32_t value;

	memcpy(&value, bytes, sizeof(value));
	return value;
}

static uint16_t extract_be16(const uint8_t *bytes)
{
	return sys_get_be16(bytes);
}

static uint8_t checksum(const uint8_t *bytes, size_t len)
{
	uint8_t value = 0;

	for (size_t i = 0; i < len; i++) {
		value ^= bytes[i];
	}

	return (uint8_t)~value;
}

static void reset_target(struct mr60bha2_target *target)
{
	memset(target, 0, sizeof(*target));
	target->x = NAN;
	target->y = NAN;
	target->distance = NAN;
	target->angle = NAN;
	target->speed = NAN;
}

static void reset_snapshot(struct mr60bha2_snapshot *snapshot)
{
	memset(snapshot, 0, sizeof(*snapshot));

	snapshot->heart_rate = NAN;
	snapshot->breath_rate = NAN;
	snapshot->distance = NAN;
	snapshot->raw_distance = NAN;
	snapshot->total_phase = NAN;
	snapshot->breath_phase = NAN;
	snapshot->heart_phase = NAN;
	snapshot->target_x = NAN;
	snapshot->target_y = NAN;
	snapshot->target_distance = NAN;
	snapshot->target_angle = NAN;
	snapshot->target_speed = NAN;
	for (size_t i = 0; i < MR60BHA2_MAX_TARGETS; i++) {
		reset_target(&snapshot->targets[i]);
	}

	snprintf(snapshot->presence_source, sizeof(snapshot->presence_source), "waiting");
	snprintf(snapshot->target_source, sizeof(snapshot->target_source), "waiting");
}

static bool parse_targets(struct mr60bha2_target *targets, uint8_t *target_count,
			  const uint8_t *data, size_t data_len)
{
	if (data_len < sizeof(uint32_t)) {
		return false;
	}

	const uint32_t count = extract_u32(data);
	const uint8_t *cursor = data + sizeof(uint32_t);
	const uint8_t *end = data + data_len;
	const size_t max_targets = MIN((size_t)count, (size_t)MR60BHA2_MAX_TARGETS);

	if (count > UINT8_MAX) {
		return false;
	}

	for (size_t i = 0; i < MR60BHA2_MAX_TARGETS; i++) {
		reset_target(&targets[i]);
	}

	for (size_t i = 0; i < count; i++) {
		if ((size_t)(end - cursor) < sizeof(float) * 2 + sizeof(uint32_t) * 2) {
			return false;
		}

		const float raw_x = extract_float(cursor);
		cursor += sizeof(float);
		const float raw_y = extract_float(cursor);
		cursor += sizeof(float);
		const int32_t dop_index = (int32_t)extract_u32(cursor);
		cursor += sizeof(uint32_t);
		const int32_t cluster_index = (int32_t)extract_u32(cursor);
		cursor += sizeof(uint32_t);

		if (i < max_targets) {
			struct mr60bha2_target *target = &targets[i];

			target->valid = true;
			target->x = fabsf(raw_x) > 10.0f ? raw_x / 100.0f : raw_x;
			target->y = fabsf(raw_y) > 10.0f ? raw_y / 100.0f : raw_y;
			target->distance = hypotf(target->x, target->y);
			target->angle = atan2f(target->x, target->y) * 180.0f / MR60BHA2_PI;
			target->speed = ((float)dop_index * 17.28f) / 100.0f;
			target->dop_index = dop_index;
			target->cluster_index = cluster_index;
		}
	}

	*target_count = (uint8_t)max_targets;
	return true;
}

static bool handle_type(struct mr60bha2_snapshot *snapshot, uint16_t type,
			const uint8_t *data, size_t data_len)
{
	switch (type) {
	case MR60BHA2_TYPE_HEART_BREATH_PHASE:
		if (data_len < sizeof(float) * 3) {
			return false;
		}
		snapshot->total_phase = extract_float(data);
		snapshot->breath_phase = extract_float(data + sizeof(float));
		snapshot->heart_phase = extract_float(data + sizeof(float) * 2);
		return true;
	case MR60BHA2_TYPE_BREATH_RATE:
		if (data_len < sizeof(float)) {
			return false;
		}
		snapshot->breath_rate = extract_float(data);
		return true;
	case MR60BHA2_TYPE_HEART_RATE:
		if (data_len < sizeof(float)) {
			return false;
		}
		snapshot->heart_rate = extract_float(data);
		return true;
	case MR60BHA2_TYPE_DISTANCE:
		if (data_len < sizeof(uint32_t) + sizeof(float)) {
			return false;
		}
		snapshot->raw_distance = extract_float(data + sizeof(uint32_t));
		snapshot->distance = snapshot->raw_distance > 6.0f
					      ? snapshot->raw_distance / 100.0f
					      : snapshot->raw_distance;
		if (snapshot->distance < 0.05f || snapshot->distance > 6.0f) {
			snapshot->distance = NAN;
		}
		return true;
	case MR60BHA2_TYPE_HUMAN_DETECTION:
		if (data_len < 1) {
			return false;
		}
		snapshot->presence = data[0] != 0;
		snapshot->presence_valid = true;
		snprintf(snapshot->presence_source, sizeof(snapshot->presence_source), "sensor");
		return true;
	case MR60BHA2_TYPE_POINT_CLOUD:
	case MR60BHA2_TYPE_TARGET_INFO:
		if (!parse_targets(snapshot->targets, &snapshot->target_count, data, data_len)) {
			return false;
		}
		snapshot->target_valid = true;
		snprintf(snapshot->target_source, sizeof(snapshot->target_source), "sensor");
		if (snapshot->target_count > 0 && snapshot->targets[0].valid) {
			snapshot->target_x = snapshot->targets[0].x;
			snapshot->target_y = snapshot->targets[0].y;
			snapshot->target_distance = snapshot->targets[0].distance;
			snapshot->target_angle = snapshot->targets[0].angle;
			snapshot->target_speed = snapshot->targets[0].speed;
		}
		return true;
	case MR60BHA2_TYPE_FIRMWARE:
		if (data_len < sizeof(uint32_t)) {
			return false;
		}
		snapshot->firmware_raw = extract_u32(data);
		snapshot->firmware_project = (uint8_t)(snapshot->firmware_raw & 0xFF);
		snapshot->firmware_major = (uint8_t)((snapshot->firmware_raw >> 8) & 0xFF);
		snapshot->firmware_sub = (uint8_t)((snapshot->firmware_raw >> 16) & 0xFF);
		snapshot->firmware_modified = (uint8_t)((snapshot->firmware_raw >> 24) & 0xFF);
		snapshot->firmware_valid = true;
		return true;
	default:
		return false;
	}
}

static bool process_frame(struct mr60bha2_device *dev, const struct mr60bha2_frame *frame)
{
	const uint8_t *bytes = frame->data;
	const size_t len = frame->len;

	if (len < MR60BHA2_FRAME_HEADER_LEN + MR60BHA2_FRAME_CHECKSUM_LEN) {
		return false;
	}

	const uint16_t data_len = extract_be16(&bytes[3]);
	const uint16_t type = extract_be16(&bytes[5]);

	if (bytes[0] != MR60BHA2_FRAME_SOF) {
		return false;
	}
	if (len != (size_t)MR60BHA2_FRAME_HEADER_LEN + data_len + MR60BHA2_FRAME_CHECKSUM_LEN) {
		return false;
	}
	if (checksum(bytes, MR60BHA2_FRAME_HEADER_LEN - MR60BHA2_FRAME_CHECKSUM_LEN) != bytes[7]) {
		return false;
	}
	if (checksum(&bytes[MR60BHA2_FRAME_HEADER_LEN], data_len) !=
	    bytes[MR60BHA2_FRAME_HEADER_LEN + data_len]) {
		return false;
	}

	return handle_type(&dev->snapshot, type, &bytes[MR60BHA2_FRAME_HEADER_LEN], data_len);
}

int mr60bha2_init(struct mr60bha2_device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	memset(dev, 0, sizeof(*dev));
	mr60bha2_parser_init(&dev->parser);
	reset_snapshot(&dev->snapshot);
	return 0;
}

enum mr60bha2_parse_result mr60bha2_process_byte(struct mr60bha2_device *dev, uint8_t byte)
{
	if (dev == NULL) {
		return MR60BHA2_PARSE_ERROR;
	}

	dev->stats.bytes_seen++;

	const enum mr60bha2_parse_result result = mr60bha2_parser_process_byte(&dev->parser, byte);

	if (result == MR60BHA2_PARSE_FRAME_READY) {
		struct mr60bha2_frame frame;

		if (mr60bha2_parser_take_frame(&dev->parser, &frame) && process_frame(dev, &frame)) {
			dev->stats.frames_seen++;
		} else {
			dev->stats.parse_errors++;
			return MR60BHA2_PARSE_ERROR;
		}
	} else if (result == MR60BHA2_PARSE_ERROR) {
		dev->stats.parse_errors++;
	}

	return result;
}

void mr60bha2_get_stats(const struct mr60bha2_device *dev, struct mr60bha2_stats *out)
{
	if (dev == NULL || out == NULL) {
		return;
	}

	*out = dev->stats;
}

void mr60bha2_snapshot_take(struct mr60bha2_device *dev, struct mr60bha2_snapshot *out)
{
	if (dev == NULL || out == NULL) {
		return;
	}

	*out = dev->snapshot;
	dev->snapshot.heart_rate = NAN;
	dev->snapshot.breath_rate = NAN;
	dev->snapshot.distance = NAN;
	dev->snapshot.raw_distance = NAN;
	dev->snapshot.total_phase = NAN;
	dev->snapshot.breath_phase = NAN;
	dev->snapshot.heart_phase = NAN;
	dev->snapshot.presence_valid = false;
	dev->snapshot.target_valid = false;
	dev->snapshot.firmware_valid = false;
	for (size_t i = 0; i < MR60BHA2_MAX_TARGETS; i++) {
		dev->snapshot.targets[i].valid = false;
	}
}
