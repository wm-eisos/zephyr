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

#include <zephyr/drivers/sensor/wsen_hids_2525020210002.h>

LOG_MODULE_REGISTER(MAIN);

#define MAX_TEST_TIME 50000
#define SLEEPTIME     500

static void print_data(const struct device *hids_2525020210002)
{
	struct sensor_value temp_value;
	struct sensor_value humd_value;

	if (sensor_channel_get(hids_2525020210002, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	if (sensor_channel_get(hids_2525020210002, SENSOR_CHAN_HUMIDITY, &humd_value) < 0) {
		LOG_ERR("Humidity channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_float(&temp_value));

	LOG_INF("Humidity (%%): %f\n", sensor_value_to_float(&humd_value));
}

int main(void)
{
	const struct device *const hids_2525020210002 = DEVICE_DT_GET(DT_NODELABEL(hids));

	if (!device_is_ready(hids_2525020210002)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	struct sensor_value precision;

	precision.val1 = hids_2525020210002_precision_Low;
	precision.val2 = 0;

	if (sensor_attr_set(hids_2525020210002, SENSOR_CHAN_ALL,
			    SENSOR_ATTR_WSEN_HIDS_2525020210002_PRECISION, &precision) < 0) {
		LOG_ERR("Failed to set precision.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	int32_t remaining_test_time = MAX_TEST_TIME;

	do {
		if (sensor_sample_fetch(hids_2525020210002) < 0) {
			LOG_INF("sample update error.\n");
		} else {
			print_data(hids_2525020210002);
		}

		/* wait a while */
		k_sleep(K_MSEC(SLEEPTIME));

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	return 0;
}
