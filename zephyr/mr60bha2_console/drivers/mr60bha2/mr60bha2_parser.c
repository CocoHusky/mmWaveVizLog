/* SPDX-License-Identifier: MIT */

#include "mr60bha2_parser.h"

#include <string.h>

#define MR60BHA2_SOF0 0x53
#define MR60BHA2_SOF1 0x59
#define MR60BHA2_EOF0 0x54
#define MR60BHA2_EOF1 0x43
#define MR60BHA2_MIN_FRAME_LEN 8

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
		if (!parser->saw_sync0) {
			parser->saw_sync0 = byte == MR60BHA2_SOF0;
			return MR60BHA2_PARSE_NEED_MORE;
		}

		if (byte != MR60BHA2_SOF1) {
			parser->saw_sync0 = byte == MR60BHA2_SOF0;
			return MR60BHA2_PARSE_NEED_MORE;
		}

		parser->collecting = true;
		parser->frame.len = 0;
		parser->frame.data[parser->frame.len++] = MR60BHA2_SOF0;
		parser->frame.data[parser->frame.len++] = MR60BHA2_SOF1;
		return MR60BHA2_PARSE_NEED_MORE;
	}

	if (parser->frame.len >= sizeof(parser->frame.data)) {
		reset_frame(parser);
		return MR60BHA2_PARSE_ERROR;
	}

	parser->frame.data[parser->frame.len++] = byte;

	if (parser->frame.len >= MR60BHA2_MIN_FRAME_LEN &&
	    parser->frame.data[parser->frame.len - 2] == MR60BHA2_EOF0 &&
	    parser->frame.data[parser->frame.len - 1] == MR60BHA2_EOF1) {
		parser->collecting = false;
		parser->saw_sync0 = false;
		return MR60BHA2_PARSE_FRAME_READY;
	}

	return MR60BHA2_PARSE_NEED_MORE;
}

bool mr60bha2_parser_take_frame(struct mr60bha2_parser *parser, struct mr60bha2_frame *out)
{
	if (parser == NULL || out == NULL || parser->collecting ||
	    parser->frame.len < MR60BHA2_MIN_FRAME_LEN) {
		return false;
	}

	*out = parser->frame;
	parser->frame.len = 0;
	return true;
}
