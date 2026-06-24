/* SPDX-License-Identifier: MIT */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "mr60bha2.h"
#include "mr60bha2_parser.h"

#define HEART_RATE_FILE TEST_DATA_DIR "/heart_rate_frame.bin"
#define BREATH_RATE_FILE TEST_DATA_DIR "/breath_rate_frame.bin"
#define DISTANCE_FILE TEST_DATA_DIR "/distance_frame.bin"
#define PRESENCE_FILE TEST_DATA_DIR "/presence_frame.bin"
#define TARGET_ONE_FILE TEST_DATA_DIR "/target_one_frame.bin"
#define BAD_CHECKSUM_FILE TEST_DATA_DIR "/bad_checksum_frame.bin"

static uint8_t checksum(const uint8_t *bytes, size_t len)
{
	uint8_t value = 0;

	for (size_t i = 0; i < len; i++) {
		value ^= bytes[i];
	}

	return (uint8_t)~value;
}

static size_t read_fixture(const char *path, uint8_t *buf, size_t buf_len)
{
	FILE *f = fopen(path, "rb");
	size_t len;

	zassert_not_null(f, "failed to open %s", path);
	len = fread(buf, 1, buf_len, f);
	fclose(f);
	zassert_true(len > 0, "fixture %s was empty", path);
	return len;
}

static void assert_fixture_decodes_to_snapshot(
	const char *path,
	void (*assert_snapshot)(const struct mr60bha2_snapshot *))
{
	struct mr60bha2_device device;
	uint8_t bytes[MR60BHA2_FRAME_MAX_LEN];
	size_t len;
	enum mr60bha2_parse_result result = MR60BHA2_PARSE_NEED_MORE;

	len = read_fixture(path, bytes, sizeof(bytes));
	zassert_equal(mr60bha2_init(&device), 0);
	for (size_t i = 0; i < len; i++) {
		result = mr60bha2_process_byte(&device, bytes[i]);
		if (i + 1U == len) {
			zassert_equal(result, MR60BHA2_PARSE_FRAME_READY);
		} else {
			zassert_equal(result, MR60BHA2_PARSE_NEED_MORE);
		}
	}
	zassert_equal(device.stats.frames_seen, 1U);
	zassert_equal(device.stats.parse_errors, 0U);
	{
		struct mr60bha2_snapshot snapshot;

		mr60bha2_snapshot_take(&device, &snapshot);
		assert_snapshot(&snapshot);
	}
}

static void assert_heart_rate_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	zassert_true(fabsf(snapshot->heart_rate - 72.5f) <= 0.001f);
	zassert_true(isnan(snapshot->breath_rate));
	zassert_false(snapshot->presence_valid);
	zassert_false(snapshot->target_valid);
}

static void assert_breath_rate_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	zassert_true(fabsf(snapshot->breath_rate - 15.2f) <= 0.001f);
	zassert_true(isnan(snapshot->heart_rate));
	zassert_false(snapshot->presence_valid);
	zassert_false(snapshot->target_valid);
}

static void assert_distance_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	zassert_true(fabsf(snapshot->raw_distance - 123.4f) <= 0.001f);
	zassert_true(fabsf(snapshot->distance - 1.234f) <= 0.001f);
	zassert_false(snapshot->presence_valid);
	zassert_false(snapshot->target_valid);
}

static void assert_presence_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	zassert_true(snapshot->presence_valid);
	zassert_true(snapshot->presence);
	zassert_str_equal(snapshot->presence_source, "sensor");
	zassert_false(snapshot->target_valid);
}

static void assert_target_snapshot(const struct mr60bha2_snapshot *snapshot)
{
	zassert_true(snapshot->target_valid);
	zassert_equal(snapshot->target_count, 1);
	zassert_true(fabsf(snapshot->target_x - 1.2f) <= 0.001f);
	zassert_true(fabsf(snapshot->target_y - 2.4f) <= 0.001f);
	zassert_true(fabsf(snapshot->target_distance - 2.683f) <= 0.01f);
	zassert_true(fabsf(snapshot->target_angle - 26.565f) <= 0.01f);
	zassert_true(fabsf(snapshot->target_speed - 0.864f) <= 0.001f);
	zassert_str_equal(snapshot->target_source, "sensor");
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

ZTEST(mr60bha2_parser, accepts_saved_heart_rate_frame)
{
	assert_fixture_decodes_to_snapshot(HEART_RATE_FILE, assert_heart_rate_snapshot);
}

ZTEST(mr60bha2_parser, accepts_saved_breath_rate_frame)
{
	assert_fixture_decodes_to_snapshot(BREATH_RATE_FILE, assert_breath_rate_snapshot);
}

ZTEST(mr60bha2_parser, accepts_saved_distance_frame)
{
	assert_fixture_decodes_to_snapshot(DISTANCE_FILE, assert_distance_snapshot);
}

ZTEST(mr60bha2_parser, accepts_saved_presence_frame)
{
	assert_fixture_decodes_to_snapshot(PRESENCE_FILE, assert_presence_snapshot);
}

ZTEST(mr60bha2_parser, accepts_saved_target_frame)
{
	assert_fixture_decodes_to_snapshot(TARGET_ONE_FILE, assert_target_snapshot);
}

ZTEST(mr60bha2_parser, rejects_bad_checksum_frame)
{
	struct mr60bha2_device device;
	uint8_t bytes[MR60BHA2_FRAME_MAX_LEN];
	size_t len;

	len = read_fixture(BAD_CHECKSUM_FILE, bytes, sizeof(bytes));
	zassert_equal(mr60bha2_init(&device), 0);
	for (size_t i = 0; i < len - 1U; i++) {
		zassert_equal(mr60bha2_process_byte(&device, bytes[i]),
			      MR60BHA2_PARSE_NEED_MORE);
	}
	zassert_equal(mr60bha2_process_byte(&device, bytes[len - 1U]), MR60BHA2_PARSE_ERROR);
	zassert_equal(device.stats.frames_seen, 0U);
	zassert_equal(device.stats.parse_errors, 1U);
}

ZTEST_SUITE(mr60bha2_parser, NULL, NULL, NULL, NULL, NULL);
