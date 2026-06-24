/* SPDX-License-Identifier: MIT */

#ifndef MR60BHA2_H
#define MR60BHA2_H

#include <stdbool.h>
#include <stdint.h>

#include "mr60bha2_parser.h"

#define MR60BHA2_MAX_TARGETS 3
#define MR60BHA2_SOURCE_LEN 16

struct mr60bha2_target {
	bool valid;
	float x;
	float y;
	float distance;
	float angle;
	float speed;
	int32_t dop_index;
	int32_t cluster_index;
};

struct mr60bha2_snapshot {
	float heart_rate;
	float breath_rate;
	float distance;
	float raw_distance;
	float total_phase;
	float breath_phase;
	float heart_phase;
	bool presence;
	bool presence_valid;
	char presence_source[MR60BHA2_SOURCE_LEN];
	bool target_valid;
	char target_source[MR60BHA2_SOURCE_LEN];
	uint8_t target_count;
	float target_x;
	float target_y;
	float target_distance;
	float target_angle;
	float target_speed;
	struct mr60bha2_target targets[MR60BHA2_MAX_TARGETS];
	bool firmware_valid;
	uint32_t firmware_raw;
	uint8_t firmware_project;
	uint8_t firmware_major;
	uint8_t firmware_sub;
	uint8_t firmware_modified;
};

struct mr60bha2_stats {
	uint32_t bytes_seen;
	uint32_t frames_seen;
	uint32_t parse_errors;
};

struct mr60bha2_device {
	struct mr60bha2_parser parser;
	struct mr60bha2_stats stats;
	struct mr60bha2_snapshot snapshot;
};

int mr60bha2_init(struct mr60bha2_device *dev);
enum mr60bha2_parse_result mr60bha2_process_byte(struct mr60bha2_device *dev, uint8_t byte);
void mr60bha2_get_stats(const struct mr60bha2_device *dev, struct mr60bha2_stats *out);
void mr60bha2_snapshot_take(struct mr60bha2_device *dev, struct mr60bha2_snapshot *out);

#endif
