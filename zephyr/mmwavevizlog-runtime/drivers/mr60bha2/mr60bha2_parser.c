/* SPDX-License-Identifier: MIT */

#include "mr60bha2_parser.h"

#include <string.h>

#define MR60BHA2_SOF 0x01
#define MR60BHA2_FRAME_HEADER_LEN 8
#define MR60BHA2_FRAME_CHECKSUM_LEN 1

static void reset_frame(struct mr60bha2_parser *parser)
{
	parser->frame.len = 0;
	parser->saw_sync0 = false;
	parser->collecting = false;
}

void mr60bha2_parser_init(struct mr60bha2_parser *parser)
{
	memset(parser, 0, sizeof(*parser));
}

enum mr60bha2_parse_result mr60bha2_parser_process_byte(struct mr60bha2_parser *parser,
							uint8_t byte)
{
	if (parser == NULL) {
		return MR60BHA2_PARSE_ERROR;
	}

	if (!parser->collecting) {
		if (byte != MR60BHA2_SOF) {
			return MR60BHA2_PARSE_NEED_MORE;
		}

		parser->collecting = true;
		parser->saw_sync0 = true;
		parser->frame.len = 0;
		parser->frame.data[parser->frame.len++] = MR60BHA2_SOF;
		return MR60BHA2_PARSE_NEED_MORE;
	}

	if (parser->frame.len >= sizeof(parser->frame.data)) {
		reset_frame(parser);
		return MR60BHA2_PARSE_ERROR;
	}

	parser->frame.data[parser->frame.len++] = byte;

	if (parser->frame.len >= MR60BHA2_FRAME_HEADER_LEN) {
		const size_t data_len = ((size_t)parser->frame.data[3] << 8) |
					(size_t)parser->frame.data[4];
		const size_t total_len = MR60BHA2_FRAME_HEADER_LEN + data_len +
					 MR60BHA2_FRAME_CHECKSUM_LEN;

		if (total_len > sizeof(parser->frame.data)) {
			reset_frame(parser);
			return MR60BHA2_PARSE_ERROR;
		}

		if (parser->frame.len == total_len) {
			parser->collecting = false;
			parser->saw_sync0 = false;
			return MR60BHA2_PARSE_FRAME_READY;
		}
	}

	return MR60BHA2_PARSE_NEED_MORE;
}

bool mr60bha2_parser_take_frame(struct mr60bha2_parser *parser, struct mr60bha2_frame *out)
{
	if (parser == NULL || out == NULL || parser->collecting ||
	    parser->frame.len < MR60BHA2_FRAME_HEADER_LEN + MR60BHA2_FRAME_CHECKSUM_LEN) {
		return false;
	}

	*out = parser->frame;
	parser->frame.len = 0;
	return true;
}
