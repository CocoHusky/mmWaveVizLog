/* SPDX-License-Identifier: MIT */

#include "ota_control.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/dfu/flash_img.h>
#include <zephyr/dfu/mcuboot.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(vislog_ota, LOG_LEVEL_INF);

struct vislog_ota_state {
	struct flash_img_context flash;
	bool upload_active;
	bool upgrade_pending;
	bool reboot_pending;
	size_t bytes_written;
	int last_error;
	uint8_t upload_slot;
};

static struct vislog_ota_state ota;
static struct k_mutex ota_lock;
static struct k_work_delayable reboot_work;

static void ota_reset_locked(void)
{
	memset(&ota.flash, 0, sizeof(ota.flash));
	ota.upload_active = false;
	ota.upgrade_pending = false;
	ota.bytes_written = 0;
	ota.last_error = 0;
	ota.upload_slot = flash_img_get_upload_slot();
}

static void reboot_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	LOG_INF("Rebooting into pending OTA image");
	sys_reboot(SYS_REBOOT_COLD);
}

static void ota_append_json(char *buf, size_t buf_len)
{
	const bool confirmed = boot_is_img_confirmed();
	const int swap_type = mcuboot_swap_type();

	(void)snprintf(buf, buf_len,
		       "{\"upload_active\":%s,\"upgrade_pending\":%s,"
		       "\"reboot_pending\":%s,\"bytes_written\":%zu,"
		       "\"upload_slot\":%u,\"image_confirmed\":%s,"
		       "\"swap_type\":%d,\"last_error\":%d}",
		       ota.upload_active ? "true" : "false",
		       ota.upgrade_pending ? "true" : "false",
		       ota.reboot_pending ? "true" : "false",
		       ota.bytes_written, ota.upload_slot,
		       confirmed ? "true" : "false", swap_type,
		       ota.last_error);
}

static int ota_start_locked(void)
{
	int ret;

	if (ota.upload_active) {
		return -EBUSY;
	}

	ota.upload_slot = flash_img_get_upload_slot();
	ret = boot_erase_img_bank(ota.upload_slot);
	if (ret != 0) {
		LOG_ERR("Failed to erase OTA slot %u: %d", ota.upload_slot, ret);
		return ret;
	}

	ret = flash_img_init_id(&ota.flash, ota.upload_slot);
	if (ret != 0) {
		LOG_ERR("Failed to init OTA flash context: %d", ret);
		return ret;
	}

	ota.upload_active = true;
	ota.upgrade_pending = false;
	ota.reboot_pending = false;
	ota.bytes_written = 0;
	ota.last_error = 0;

	return 0;
}

static int ota_write_locked(const uint8_t *data, size_t len, bool flush)
{
	int ret;

	if (!ota.upload_active) {
		return -EIO;
	}

	ret = flash_img_buffered_write(&ota.flash, data, len, flush);
	if (ret == 0) {
		ota.bytes_written += len;
	}

	return ret;
}

static int ota_finish_locked(void)
{
	int ret;

	if (!ota.upload_active) {
		return -EIO;
	}

	ret = boot_request_upgrade(BOOT_UPGRADE_TEST);
	if (ret != 0) {
		LOG_ERR("Failed to mark OTA image for test boot: %d", ret);
		ota.last_error = ret;
	} else {
		ota.upgrade_pending = true;
	}

	ota.upload_active = false;
	return ret;
}

void vislog_ota_init(void)
{
	k_mutex_init(&ota_lock);
	k_work_init_delayable(&reboot_work, reboot_work_handler);

	k_mutex_lock(&ota_lock, K_FOREVER);
	ota_reset_locked();
	k_mutex_unlock(&ota_lock);
}

void vislog_ota_confirm_running_image(void)
{
	if (boot_is_img_confirmed()) {
		return;
	}

	if (boot_write_img_confirmed() == 0) {
		LOG_INF("Running OTA image confirmed");
	} else {
		LOG_WRN("Failed to confirm running image");
	}
}

static void set_json_response(struct http_response_ctx *response_ctx, const char *json)
{
	static const struct http_header headers[] = {
		{ .name = "content-type", .value = "application/json" },
		{ .name = "cache-control", .value = "no-store" },
	};

	response_ctx->status = HTTP_200_OK;
	response_ctx->headers = headers;
	response_ctx->header_count = ARRAY_SIZE(headers);
	response_ctx->body = (const uint8_t *)json;
	response_ctx->body_len = strlen(json);
	response_ctx->final_chunk = true;
}

int vislog_ota_handle_http(struct http_client_ctx *client, enum http_transaction_status status,
			   const struct http_request_ctx *request_ctx,
			   struct http_response_ctx *response_ctx)
{
	static char json[256];
	int ret = 0;

	ARG_UNUSED(client);

	if (status == HTTP_SERVER_TRANSACTION_ABORTED) {
		k_mutex_lock(&ota_lock, K_FOREVER);
		ota_reset_locked();
		k_mutex_unlock(&ota_lock);
		return 0;
	}

	if (request_ctx != NULL && request_ctx->data_len > 0 &&
	    (status == HTTP_SERVER_REQUEST_DATA_MORE || status == HTTP_SERVER_REQUEST_DATA_FINAL)) {
		k_mutex_lock(&ota_lock, K_FOREVER);
		if (!ota.upload_active) {
			ret = ota_start_locked();
		}
		if (ret == 0) {
			ret = ota_write_locked(request_ctx->data, request_ctx->data_len,
					       status == HTTP_SERVER_REQUEST_DATA_FINAL);
		}
		if (ret == 0 && status == HTTP_SERVER_REQUEST_DATA_FINAL) {
			ret = ota_finish_locked();
		}
		if (ret != 0) {
			ota.last_error = ret;
			ota.upload_active = false;
		}
		k_mutex_unlock(&ota_lock);

		if (ret != 0) {
			return ret;
		}

		if (status == HTTP_SERVER_REQUEST_DATA_MORE) {
			return 0;
		}
	}

	if (status != HTTP_SERVER_REQUEST_DATA_FINAL) {
		return 0;
	}

	if (request_ctx != NULL && request_ctx->data_len == 0) {
		/* Allow empty GET/POST handlers to fall through to status response. */
	}

	if (request_ctx == NULL || request_ctx->data_len == 0) {
		k_mutex_lock(&ota_lock, K_FOREVER);
		ota_append_json(json, sizeof(json));
		k_mutex_unlock(&ota_lock);
		set_json_response(response_ctx, json);
		return 0;
	}

	k_mutex_lock(&ota_lock, K_FOREVER);
	ota_append_json(json, sizeof(json));
	k_mutex_unlock(&ota_lock);
	set_json_response(response_ctx, json);

	return 0;
}

int vislog_ota_schedule_reboot(void)
{
	int ret = 0;

	k_mutex_lock(&ota_lock, K_FOREVER);
	if (!ota.upgrade_pending) {
		ret = -ENOENT;
	} else if (ota.reboot_pending) {
		ret = -EALREADY;
	} else {
		ota.reboot_pending = true;
		k_work_schedule(&reboot_work, K_MSEC(500));
	}
	k_mutex_unlock(&ota_lock);

	return ret;
}
