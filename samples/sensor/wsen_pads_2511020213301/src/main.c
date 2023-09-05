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
#include <zephyr/drivers/sensor/wsen_pads_2511020213301.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(MAIN);

#define MAX_TEST_TIME 50000
#define SLEEPTIME     500

static void process_sample(const struct device *pads_2511020213301)
{
	struct sensor_value temp_value;
	struct sensor_value press_value;

	if (sensor_sample_fetch(pads_2511020213301) < 0) {
		LOG_INF("sample update error.\n");
		return;
	}

	if (sensor_channel_get(pads_2511020213301, SENSOR_CHAN_AMBIENT_TEMP, &temp_value) < 0) {
		LOG_ERR("Temperature channel read error.\n");
		return;
	}

	if (sensor_channel_get(pads_2511020213301, SENSOR_CHAN_PRESS, &press_value) < 0) {
		LOG_ERR("Pressure channel read error.\n");
		return;
	}

	LOG_INF("Temperature (Celsius): %f\n", sensor_value_to_double(&temp_value));

	LOG_INF("Pressure (kPa): %f\n", sensor_value_to_double(&press_value));
}
#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD

static void
pads_2511020213301_pressure_high_interrupt_handler(const struct device *pads_2511020213301,
						   const struct sensor_trigger *trig)
{
	LOG_INF("Pressure above threshold.\n");
}

static void
pads_2511020213301_pressure_low_interrupt_handler(const struct device *pads_2511020213301,
						  const struct sensor_trigger *trig)
{
	LOG_INF("Pressure below threshold.\n");
}

void pressure_high_trigger(const struct device *pads_2511020213301)
{

	struct sensor_value threshold;

	sensor_value_from_double(&threshold, 1000.0);

	sensor_attr_set(pads_2511020213301, SENSOR_CHAN_PRESS,
			SENSOR_ATTR_WSEN_PADS_2511020213301_REFERENCE_POINT, NULL);

	sensor_attr_set(pads_2511020213301, SENSOR_CHAN_PRESS, SENSOR_ATTR_UPPER_THRESH,
			&threshold);

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_UPPER,
		.chan = SENSOR_CHAN_PRESS,
	};

	if (sensor_trigger_set(pads_2511020213301, &trig,
			       pads_2511020213301_pressure_high_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}

void pressure_low_trigger(const struct device *pads_2511020213301)
{

	struct sensor_value threshold;

	sensor_value_from_double(&threshold, 1000.0);

	sensor_attr_set(pads_2511020213301, SENSOR_CHAN_PRESS,
			SENSOR_ATTR_WSEN_PADS_2511020213301_REFERENCE_POINT, NULL);

	sensor_attr_set(pads_2511020213301, SENSOR_CHAN_PRESS, SENSOR_ATTR_LOWER_THRESH,
			&threshold);

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_WSEN_PADS_2511020213301_THRESHOLD_LOWER,
		.chan = SENSOR_CHAN_PRESS,
	};

	if (sensor_trigger_set(pads_2511020213301, &trig,
			       pads_2511020213301_pressure_low_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}
#else
static void pads_2511020213301_data_ready_interrupt_handler(const struct device *pads_2511020213301,
							    const struct sensor_trigger *trig)
{
	process_sample(pads_2511020213301);
}

void data_ready_trigger(const struct device *pads_2511020213301)
{

	struct sensor_trigger trig = {
		.type = SENSOR_TRIG_DATA_READY,
		.chan = SENSOR_CHAN_PRESS,
	};

	if (sensor_trigger_set(pads_2511020213301, &trig,
			       pads_2511020213301_data_ready_interrupt_handler) < 0) {
		LOG_ERR("Failed to configure trigger.");
	}
}
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

int main(void)
{
	const struct device *const pads_2511020213301 = DEVICE_DT_GET(DT_NODELABEL(pads));

	if (!device_is_ready(pads_2511020213301)) {
		LOG_ERR("sensor: device not ready.\n");
		return 0;
	}

	k_sleep(K_MSEC(SLEEPTIME));

	if (IS_ENABLED(CONFIG_WSEN_PADS_2511020213301_TRIGGER)) {

#ifdef CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD
		pressure_high_trigger(pads_2511020213301);
		pressure_low_trigger(pads_2511020213301);
#else
		data_ready_trigger(pads_2511020213301);
#endif /* CONFIG_WSEN_PADS_2511020213301_PRESSURE_THRESHOLD */

	} else {
		/* polling mode */

		int32_t remaining_test_time = MAX_TEST_TIME;
		
		do {
			process_sample(pads_2511020213301);

			/* wait a while */
			k_sleep(K_MSEC(SLEEPTIME));

			remaining_test_time -= SLEEPTIME;
		} while (remaining_test_time > 0);
	}

	k_sleep(K_FOREVER);
	return 0;
}
