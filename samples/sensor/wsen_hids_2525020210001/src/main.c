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

static void process_sample(const struct device *hids_2525020210001)
{
	struct sensor_value temp_value;
	struct sensor_value humd_value;

	if (sensor_sample_fetch(hids_2525020210001) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(hids_2525020210001, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	if (sensor_channel_get(hids_2525020210001, SENSOR_CHAN_HUMIDITY, &humd_value) < 0) {
		LOG_ERR("Humidity channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_double(&temp_value));

	LOG_INF("Humidity (%%): %f\n", sensor_value_to_double(&humd_value));
}

static void hids_2525020210001_data_ready_handler(const struct device *hids_2525020210001,
						  const struct sensor_trigger *trig)
{
	process_sample(hids_2525020210001);
}

int main(void)
{
	const struct device *const hids_2525020210001 = DEVICE_DT_GET(DT_NODELABEL(hids));

	if (!device_is_ready(hids_2525020210001)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	#if defined(CONFIG_WSEN_HIDS_2525020210001_TRIGGER)
	/* CONFIG_WSEN_HIDS_2525020210001_TRIGGER */
		/* interrupt mode */
		struct sensor_trigger trig = {
			.type = SENSOR_TRIG_DATA_READY,
			.chan = SENSOR_CHAN_ALL,
		};

		if (sensor_trigger_set(hids_2525020210001, &trig,
				       hids_2525020210001_data_ready_handler) < 0) {
			LOG_ERR("Failed to configure trigger.");
		}
	#else
		/* polling mode */
		int32_t remaining_test_time = MAX_TEST_TIME;

		do {
			process_sample(hids_2525020210001);

			/* wait a while */
			k_sleep(K_MSEC(SLEEPTIME));

			remaining_test_time -= SLEEPTIME;
		} while (remaining_test_time > 0);
	#endif

	k_sleep(K_FOREVER);
	return 0;
}
