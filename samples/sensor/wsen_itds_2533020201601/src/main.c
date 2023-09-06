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

static void process_sample_temp(const struct device *itds_2533020201601)
{
	struct sensor_value temp_value;

	if (sensor_sample_fetch_chan(itds_2533020201601, SENSOR_CHAN_AMBIENT_TEMP) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(itds_2533020201601, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_float(&temp_value));
}

static void process_sample_acceleration(const struct device *itds_2533020201601)
{
	struct sensor_value accel_value[3];

	if (sensor_sample_fetch_chan(itds_2533020201601, SENSOR_CHAN_ACCEL_XYZ) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(itds_2533020201601, SENSOR_CHAN_ACCEL_XYZ, accel_value) < 0) {
		LOG_ERR("Acceleration channel read error.\n");
		return;
	}
	LOG_INF("Acceleration (g): x=%f, y=%f, z=%f\n",
		sensor_ms2_to_ug(&accel_value[0]) / 1000000.0f,
		sensor_ms2_to_ug(&accel_value[1]) / 1000000.0f,
		sensor_ms2_to_ug(&accel_value[2]) / 1000000.0f);
}

static void data_ready_interrupt_handler(const struct device *itds_2533020201601,
					 const struct sensor_trigger *trig)
{
	process_sample_acceleration(itds_2533020201601);
}

void data_ready_trigger(const struct device *itds_2533020201601)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(itds_2533020201601, &trig, data_ready_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP

static void tap_interrupt_handler(const struct device *itds_2533020201601,
				  const struct sensor_trigger *trig)
{
	LOG_INF("Single Tap.\n");
}

static void double_tap_interrupt_handler(const struct device *itds_2533020201601,
					 const struct sensor_trigger *trig)
{
	LOG_INF("Double Tap.\n");
}

void tap_trigger(const struct device *itds_2533020201601)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(itds_2533020201601, &trig, tap_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

void double_tap_trigger(const struct device *itds_2533020201601)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DOUBLE_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(itds_2533020201601, &trig, double_tap_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL

static void free_fall_interrupt_handler(const struct device *itds_2533020201601,
					const struct sensor_trigger *trig)
{
	LOG_INF("Free fall.\n");
}

void free_fall_trigger(const struct device *itds_2533020201601)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_FREEFALL,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(itds_2533020201601, &trig, free_fall_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA

static void wake_up_interrupt_handler(const struct device *itds_2533020201601,
				      const struct sensor_trigger *trig)
{
	LOG_INF("Wake up.\n");
}

void wake_up_trigger(const struct device *itds_2533020201601)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DELTA,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(itds_2533020201601, &trig, wake_up_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */

int main(void)
{
	const struct device *const itds_2533020201601 = DEVICE_DT_GET(DT_NODELABEL(itds));

	if (!device_is_ready(itds_2533020201601)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (IS_ENABLED(CONFIG_WSEN_ITDS_2533020201601_TRIGGER)) {
		/* interrupt mode */
		data_ready_trigger(itds_2533020201601);

#ifdef CONFIG_WSEN_ITDS_2533020201601_TAP
		tap_trigger(itds_2533020201601);

		double_tap_trigger(itds_2533020201601);
#endif /* CONFIG_WSEN_ITDS_2533020201601_TAP */

#ifdef CONFIG_WSEN_ITDS_2533020201601_FREEFALL
		free_fall_trigger(itds_2533020201601);
#endif /* CONFIG_WSEN_ITDS_2533020201601_FREEFALL */

#ifdef CONFIG_WSEN_ITDS_2533020201601_DELTA
		wake_up_trigger(itds_2533020201601);
#endif /* CONFIG_WSEN_ITDS_2533020201601_DELTA */

	} else {
		/* polling mode */

		int32_t remaining_test_time = MAX_TEST_TIME;
		
		do {
			process_sample_temp(itds_2533020201601);
			process_sample_acceleration(itds_2533020201601);

			/* wait a while */
			k_sleep(K_MSEC(SLEEPTIME));

			remaining_test_time -= SLEEPTIME;
		} while (remaining_test_time > 0);
	}

	k_sleep(K_FOREVER);
	return 0;
}
