/* SPDX-License-Identifier: MIT */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include "app_state.h"
#include "json_stream.h"
#include "mr60bha2.h"

#define SAMPLE_PERIOD_MS 100
#define SERIAL_PERIOD_MS 1000
#define SAMPLE_THREAD_STACK_SIZE 2048
#define SERIAL_THREAD_STACK_SIZE 4096
#define SAMPLE_THREAD_PRIORITY 5
#define SERIAL_THREAD_PRIORITY 7

static struct mr60bha2_device radar;

static void sample_thread(void *unused_a, void *unused_b, void *unused_c)
{
	ARG_UNUSED(unused_a);
	ARG_UNUSED(unused_b);
	ARG_UNUSED(unused_c);

	while (true) {
		vislog_app_state_update_simulated();
		k_sleep(K_MSEC(SAMPLE_PERIOD_MS));
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

K_THREAD_STACK_DEFINE(sample_thread_stack, SAMPLE_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(serial_thread_stack, SERIAL_THREAD_STACK_SIZE);

static struct k_thread sample_thread_data;
static struct k_thread serial_thread_data;

int main(void)
{
	vislog_app_state_init();
	mr60bha2_init(&radar);

	printk("{\"type\":\"boot\",\"app\":\"mr60bha2_console\",\"runtime\":\"zephyr\"}\n");

	k_thread_create(&sample_thread_data, sample_thread_stack,
			K_THREAD_STACK_SIZEOF(sample_thread_stack),
			sample_thread, NULL, NULL, NULL,
			SAMPLE_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sample_thread_data, "vislog_sample");

	k_thread_create(&serial_thread_data, serial_thread_stack,
			K_THREAD_STACK_SIZEOF(serial_thread_stack),
			serial_thread, NULL, NULL, NULL,
			SERIAL_THREAD_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&serial_thread_data, "vislog_serial");

	while (true) {
		k_sleep(K_SECONDS(60));
	}
}
