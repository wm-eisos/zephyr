/*
 * Copyright (c) 2023 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys_clock.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN);

#define MAX_TEST_TIME 50000
#define SLEEPTIME     500

static void process_sample(const struct device *pdus_25131308XXXXX)
{
	struct sensor_value temp_value;
	struct sensor_value press_value;

	if (sensor_sample_fetch(pdus_25131308XXXXX) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(pdus_25131308XXXXX, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	if (sensor_channel_get(pdus_25131308XXXXX, SENSOR_CHAN_PRESS, &press_value) < 0) {
		LOG_ERR("Pressure channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_double(&temp_value));

	LOG_INF("Pressure (kPa): %f\n", sensor_value_to_double(&press_value));
}

int main(void)
{
	const struct device *const pdus_25131308XXXXX = DEVICE_DT_GET(DT_NODELABEL(pdus));

	if (!device_is_ready(pdus_25131308XXXXX)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	int32_t remaining_test_time = MAX_TEST_TIME;
	
	do {
		process_sample(pdus_25131308XXXXX);

		/* wait a while */
		k_sleep(K_MSEC(SLEEPTIME));

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	k_sleep(K_FOREVER);
	return 0;
}
