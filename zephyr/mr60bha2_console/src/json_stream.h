/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_JSON_STREAM_H
#define VISLOG_JSON_STREAM_H

#include <stddef.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VISLOG_JSON_LINE_MAX 1536

int vislog_sample_to_json(const struct vislog_sample *sample, char *buf, size_t len);
void vislog_print_sample_json(const struct vislog_sample *sample);

#ifdef __cplusplus
}
#endif

#endif
