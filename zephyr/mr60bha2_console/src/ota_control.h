/* SPDX-License-Identifier: MIT */

#ifndef VISLOG_OTA_CONTROL_H
#define VISLOG_OTA_CONTROL_H

#include <stddef.h>

#include <zephyr/net/http/server.h>

#ifdef __cplusplus
extern "C" {
#endif

void vislog_ota_init(void);
void vislog_ota_confirm_running_image(void);
int vislog_ota_handle_http(struct http_client_ctx *client, enum http_transaction_status status,
			   const struct http_request_ctx *request_ctx,
			   struct http_response_ctx *response_ctx);
int vislog_ota_schedule_reboot(void);

#ifdef __cplusplus
}
#endif

#endif
