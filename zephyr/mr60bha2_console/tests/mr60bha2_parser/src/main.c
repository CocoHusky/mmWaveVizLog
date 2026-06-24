/* SPDX-License-Identifier: MIT */

#include <zephyr/ztest.h>
#include <string.h>

#include "mr60bha2_parser.h"

static uint8_t checksum(const uint8_t *bytes, size_t len)
{
	uint8_t value = 0;

	for (size_t i = 0; i < len; i++) {
		value ^= bytes[i];
	}

	return (uint8_t)~value;
}

ZTEST(mr60bha2_parser, init_starts_empty)
{
	struct mr60bha2_parser parser;

	mr60bha2_parser_init(&parser);

	zassert_equal(parser.frame.len, 0);
	zassert_false(parser.saw_sync0);
	zassert_false(parser.collecting);
}

ZTEST(mr60bha2_parser, ignores_bytes_before_sync)
{
	struct mr60bha2_parser parser;

	mr60bha2_parser_init(&parser);

	zassert_equal(mr60bha2_parser_process_byte(&parser, 0x00),
		      MR60BHA2_PARSE_NEED_MORE);
	zassert_equal(parser.frame.len, 0);
	zassert_false(parser.collecting);
}

ZTEST(mr60bha2_parser, buffers_partial_frame)
{
	struct mr60bha2_parser parser;

	mr60bha2_parser_init(&parser);

	zassert_equal(mr60bha2_parser_process_byte(&parser, 0x01),
		      MR60BHA2_PARSE_NEED_MORE);
	zassert_equal(mr60bha2_parser_process_byte(&parser, 0x80),
		      MR60BHA2_PARSE_NEED_MORE);
	zassert_true(parser.collecting);
	zassert_equal(parser.frame.len, 2);
	zassert_false(mr60bha2_parser_take_frame(&parser, &(struct mr60bha2_frame){0}));
}

ZTEST(mr60bha2_parser, detects_frame_envelope)
{
	struct mr60bha2_parser parser;
	struct mr60bha2_frame frame;
	uint8_t bytes[] = {
		0x01, 0x80, 0x00, 0x00, 0x04, 0x0A, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	bytes[7] = checksum(bytes, 7);
	bytes[11] = checksum(&bytes[8], 4);

	mr60bha2_parser_init(&parser);

	for (size_t i = 0; i < sizeof(bytes) - 1; i++) {
		zassert_equal(mr60bha2_parser_process_byte(&parser, bytes[i]),
			      MR60BHA2_PARSE_NEED_MORE);
	}

	zassert_equal(mr60bha2_parser_process_byte(&parser, bytes[sizeof(bytes) - 1]),
		      MR60BHA2_PARSE_FRAME_READY);
	zassert_true(mr60bha2_parser_take_frame(&parser, &frame));
	zassert_equal(frame.len, sizeof(bytes));
	zassert_mem_equal(frame.data, bytes, sizeof(bytes));
}

ZTEST_SUITE(mr60bha2_parser, NULL, NULL, NULL, NULL, NULL);
