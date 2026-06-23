/* SPDX-License-Identifier: MIT */

#ifndef MR60BHA2_H
#define MR60BHA2_H

#include <stdbool.h>
#include <stdint.h>

#include "mr60bha2_parser.h"

struct mr60bha2_stats {
	uint32_t bytes_seen;
	uint32_t frames_seen;
	uint32_t parse_errors;
};

struct mr60bha2_device {
	struct mr60bha2_parser parser;
	struct mr60bha2_stats stats;
};

int mr60bha2_init(struct mr60bha2_device *dev);
enum mr60bha2_parse_result mr60bha2_process_byte(struct mr60bha2_device *dev, uint8_t byte);
void mr60bha2_get_stats(const struct mr60bha2_device *dev, struct mr60bha2_stats *out);

#endif
