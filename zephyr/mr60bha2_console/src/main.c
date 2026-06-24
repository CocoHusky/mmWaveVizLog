/* SPDX-License-Identifier: MIT */

#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

#include "app_state.h"
#include "led_control.h"
#include "ota_control.h"
#include "json_stream.h"
#include "net_services.h"
#include "mr60bha2.h"

#define UART_POLL_SLEEP_MS 5
#define SERIAL_PERIOD_MS 1000
#define RADAR_THREAD_STACK_SIZE 2048
#define SERIAL_THREAD_STACK_SIZE 4096
#define RADAR_THREAD_PRIORITY 5
#define SERIAL_THREAD_PRIORITY 7

static struct mr60bha2_device radar;
static const struct device *radar_uart;

static void radar_thread(void *unused_a, void *unused_b, void *unused_c)
{
	ARG_UNUSED(unused_a);
	ARG_UNUSED(unused_b);
	ARG_UNUSED(unused_c);

	while (true) {
		uint8_t byte;

		while (uart_poll_in(radar_uart, &byte) == 0) {
			const enum mr60bha2_parse_result result = mr60bha2_process_byte(&radar, byte);

			if (result == MR60BHA2_PARSE_FRAME_READY) {
				struct mr60bha2_snapshot snapshot;
				struct vislog_sample sample;

				mr60bha2_snapshot_take(&radar, &snapshot);
				vislog_app_state_apply_radar_snapshot(&snapshot);
				vislog_app_state_copy(&sample);
				vislog_led_update_from_sample(&sample);
				vislog_ota_confirm_running_image();
			}
		}

		k_sleep(K_MSEC(UART_POLL_SLEEP_MS));
	}
}

static void serial_thread(void *unused_a, void *unused_b, void *unused_c)
{
	ARG_UNUSED(unused_a);
	ARG_UNUSED(unused_b);
	ARG_UNUSED(unused_c);

	struct vislog_sample sample;

	while (true) {
		vislog_app_state_copy(&sample);
		vislog_print_sample_json(&sample);
		k_sleep(K_MSEC(SERIAL_PERIOD_MS));
	}
}

K_THREAD_STACK_DEFINE(radar_thread_stack, RADAR_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(serial_thread_stack, SERIAL_THREAD_STACK_SIZE);

static struct k_thread radar_thread_data;
static struct k_thread serial_thread_data;

int main(void)
{
	vislog_app_state_init();
	vislog_led_init();
	vislog_ota_init();
	mr60bha2_init(&radar);
	radar_uart = DEVICE_DT_GET(DT_NODELABEL(uart0));

	if (!device_is_ready(radar_uart)) {
		printk("{\"type\":\"error\",\"message\":\"radar uart not ready\"}\n");
		return 0;
	}

	printk("{\"type\":\"boot\",\"app\":\"mr60bha2_console\",\"runtime\":\"zephyr\"}\n");

	vislog_net_services_init();

	k_thread_create(&radar_thread_data, radar_thread_stack,
			K_THREAD_STACK_SIZEOF(radar_thread_stack),
			radar_thread, NULL, NULL, NULL,
			RADAR_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&radar_thread_data, "vislog_radar");

	k_thread_create(&serial_thread_data, serial_thread_stack,
			K_THREAD_STACK_SIZEOF(serial_thread_stack),
			serial_thread, NULL, NULL, NULL,
			SERIAL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&serial_thread_data, "vislog_serial");

	while (true) {
		k_sleep(K_SECONDS(60));
	}
}
