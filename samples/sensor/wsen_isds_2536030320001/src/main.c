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

static void process_sample_temp(const struct device *isds_2536030320001)
{
	struct sensor_value temp_value;

	if (sensor_sample_fetch_chan(isds_2536030320001, SENSOR_CHAN_AMBIENT_TEMP) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(isds_2536030320001, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_float(&temp_value));
}

static void process_sample_acceleration(const struct device *isds_2536030320001)
{
	struct sensor_value accel_value[3];

	if (sensor_sample_fetch_chan(isds_2536030320001, SENSOR_CHAN_ACCEL_XYZ) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(isds_2536030320001, SENSOR_CHAN_ACCEL_XYZ, accel_value) < 0) {
		LOG_ERR("Acceleration channel read error.\n");
		return;
	}

	LOG_INF("Acceleration (g): x=%f, y=%f, z=%f\n",
		sensor_ms2_to_ug(&accel_value[0]) / 1000000.0f,
		sensor_ms2_to_ug(&accel_value[1]) / 1000000.0f,
		sensor_ms2_to_ug(&accel_value[2]) / 1000000.0f);
}

static void process_sample_gyro(const struct device *isds_2536030320001)
{
	struct sensor_value gyro_value[3];

	if (sensor_sample_fetch_chan(isds_2536030320001, SENSOR_CHAN_GYRO_XYZ) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(isds_2536030320001, SENSOR_CHAN_GYRO_XYZ, gyro_value) < 0) {
		LOG_ERR("Gyroscope channel read error.\n");
		return;
	}

	LOG_INF("Angular Rate (deg/s): x=%f, y=%f, z=%f\n",
		sensor_rad_to_10udegrees(&gyro_value[0]) / 100000.0f,
		sensor_rad_to_10udegrees(&gyro_value[1]) / 100000.0f,
		sensor_rad_to_10udegrees(&gyro_value[2]) / 100000.0f);
}

#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
static void tap_interrupt_handler(const struct device *isds_2536030320001,
				  const struct sensor_trigger *trig)
{
	LOG_INF("Single Tap.\n");
}

void tap_trigger(const struct device *isds_2536030320001)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_TAP,
		.chan = SENSOR_CHAN_ALL,
	};

	if (sensor_trigger_set(isds_2536030320001, &trig, tap_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
static void accel_data_ready_interrupt_handler(const struct device *isds_2536030320001,
					       const struct sensor_trigger *trig)
{
	process_sample_acceleration(isds_2536030320001);
}

void accel_data_ready_trigger(const struct device *isds_2536030320001)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_ACCEL_XYZ,
	};

	if (sensor_trigger_set(isds_2536030320001, &trig, accel_data_ready_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */

int main(void)
{
	const struct device *const isds_2536030320001 = DEVICE_DT_GET(DT_NODELABEL(isds));

	if (!device_is_ready(isds_2536030320001)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

#ifdef CONFIG_WSEN_ISDS_2536030320001_TRIGGER
		/* interrupt mode */
		accel_data_ready_trigger(isds_2536030320001);
#ifdef CONFIG_WSEN_ISDS_2536030320001_TAP
		tap_trigger(isds_2536030320001);
#endif /* CONFIG_WSEN_ISDS_2536030320001_TAP */

#else
		/* polling mode */
		int32_t remaining_test_time = MAX_TEST_TIME;

		do {
			process_sample_temp(isds_2536030320001);
			process_sample_acceleration(isds_2536030320001);
			process_sample_gyro(isds_2536030320001);
			/* wait a while */
			k_sleep(K_MSEC(SLEEPTIME));

			remaining_test_time -= SLEEPTIME;
		} while (remaining_test_time > 0);
#endif /* CONFIG_WSEN_ISDS_2536030320001_TRIGGER */

	k_sleep(K_FOREVER);
	return 0;
}
