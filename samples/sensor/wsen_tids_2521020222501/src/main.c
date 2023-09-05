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
#include <zephyr/drivers/sensor/wsen_tids_2521020222501.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN);

#define MAX_TEST_TIME 50000
#define SLEEPTIME     500

static void process_sample(const struct device *tids_2521020222501)
{
	struct sensor_value temp_value;

	if (sensor_sample_fetch(tids_2521020222501) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(tids_2521020222501, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_double(&temp_value));
}

static void tids_2521020222501_low_temperature_handler(const struct device *tids_2521020222501,
						       const struct sensor_trigger *trig)
{
	LOG_INF("Low temperature threshold\n");

	if (sensor_trigger_set(tids_2521020222501, trig, NULL) < 0) {
		LOG_ERR("Failed to remove trigger.");
	}
}

static void tids_2521020222501_high_temperature_handler(const struct device *tids_2521020222501,
							const struct sensor_trigger *trig)
{
	LOG_INF("High temperature threshold\n");

	if (sensor_trigger_set(tids_2521020222501, trig, NULL) < 0) {
		LOG_ERR("Failed to remove trigger.");
	}
}

void temperature_low_trigger(const struct device *tids_2521020222501)
{

	struct sensor_value threshold;

	sensor_value_from_double(&threshold, 23.0);

	sensor_attr_set(tids_2521020222501, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_LOWER_THRESH,
			&threshold);

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_LOWER,
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
	};

	if (sensor_trigger_set(tids_2521020222501, &trig,
			       tids_2521020222501_low_temperature_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

void temperature_high_trigger(const struct device *tids_2521020222501)
{

	struct sensor_value threshold;

	sensor_value_from_double(&threshold, 27.0);

	sensor_attr_set(tids_2521020222501, SENSOR_CHAN_AMBIENT_TEMP, SENSOR_ATTR_UPPER_THRESH,
			&threshold);

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_WSEN_TIDS_2521020222501_THRESHOLD_UPPER,
		.chan = SENSOR_CHAN_AMBIENT_TEMP,
	};

	if (sensor_trigger_set(tids_2521020222501, &trig,
			       tids_2521020222501_high_temperature_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

int main(void)
{
	const struct device *const tids_2521020222501 = DEVICE_DT_GET(DT_NODELABEL(tids));

	if (!device_is_ready(tids_2521020222501)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (IS_ENABLED(CONFIG_WSEN_TIDS_2521020222501_TRIGGER)) {
		temperature_high_trigger(tids_2521020222501);
		temperature_low_trigger(tids_2521020222501);
	}

	/* polling mode */
	int32_t remaining_test_time = MAX_TEST_TIME;

	do {
		process_sample(tids_2521020222501);

		/* wait a while */
		k_sleep(K_MSEC(SLEEPTIME));

		remaining_test_time -= SLEEPTIME;
	} while (remaining_test_time > 0);

	k_sleep(K_FOREVER);
	return 0;
}
