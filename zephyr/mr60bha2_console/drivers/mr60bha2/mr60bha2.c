/* SPDX-License-Identifier: MIT */

#include "mr60bha2.h"

#include <errno.h>
#include <string.h>

int mr60bha2_init(struct mr60bha2_device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	memset(dev, 0, sizeof(*dev));
	mr60bha2_parser_init(&dev->parser);
	return 0;
}

enum mr60bha2_parse_result mr60bha2_process_byte(struct mr60bha2_device *dev, uint8_t byte)
{
	if (dev == NULL) {
		return MR60BHA2_PARSE_ERROR;
	}

	dev->stats.bytes_seen++;

	const enum mr60bha2_parse_result result =
		mr60bha2_parser_process_byte(&dev->parser, byte);

	if (result == MR60BHA2_PARSE_FRAME_READY) {
		dev->stats.frames_seen++;
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
