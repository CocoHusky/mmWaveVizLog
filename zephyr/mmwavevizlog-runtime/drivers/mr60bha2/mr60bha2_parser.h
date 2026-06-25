/* SPDX-License-Identifier: MIT */

#ifndef MR60BHA2_PARSER_H
#define MR60BHA2_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MR60BHA2_FRAME_MAX_LEN 128

enum mr60bha2_parse_result {
	MR60BHA2_PARSE_NEED_MORE = 0,
	MR60BHA2_PARSE_FRAME_READY = 1,
	MR60BHA2_PARSE_ERROR = -1,
};

struct mr60bha2_frame {
	uint8_t data[MR60BHA2_FRAME_MAX_LEN];
	size_t len;
};

struct mr60bha2_parser {
	struct mr60bha2_frame frame;
	bool saw_sync0;
	bool collecting;
};

void mr60bha2_parser_init(struct mr60bha2_parser *parser);
enum mr60bha2_parse_result mr60bha2_parser_process_byte(struct mr60bha2_parser *parser,
							uint8_t byte);
bool mr60bha2_parser_take_frame(struct mr60bha2_parser *parser, struct mr60bha2_frame *out);

#endif
