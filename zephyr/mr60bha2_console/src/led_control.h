/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_LED_CONTROL_H
#define VISLOG_LED_CONTROL_H

#include <stddef.h>
#include <stdint.h>

#include "app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

void vislog_led_init(void);
void vislog_led_update_from_sample(const struct vislog_sample *sample);
int vislog_led_apply_json(const char *body);

#ifdef __cplusplus
}
#endif

#endif
