/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_WSEN_GCDS_2526101040301_H_
#define ZEPHYR_DRIVERS_SENSOR_WSEN_GCDS_2526101040301_H_

#include <stdbool.h>

#include <zephyr/drivers/sensor.h>

#include <weplatform.h>

#include "WSEN_GCDS_2526101040301_hal.h"

#include <zephyr/drivers/i2c.h>

struct gcds_2526101040301_data {
	/* WE sensor interface configuration */
	WE_sensorInterface_t sensor_interface;

	/* Last temperature sample */
	uint16_t co2_data;
	int32_t temperature_data;
	uint32_t humidity_data;
};

typedef enum {
	gcds_2526101040301_op_mode_single_shot,
	gcds_2526101040301_op_mode_low_power_periodic,
	gcds_2526101040301_op_mode_periodic
} gcds_2526101040301_op_mode_t;

struct gcds_2526101040301_config {
	union {
		const struct i2c_dt_spec i2c;
	} bus_cfg;

	const gcds_2526101040301_op_mode_t operation_mode;
	const uint32_t temperature_offset;
	const uint16_t altitude;
	const struct sensor_value ambient_pressure;
};

/* GCDS Sensor API functions*/
static int gcds_2526101040301_sample_fetch(const struct device *dev, enum sensor_channel chan);

/* GCDS configuration functions */
static bool gcds_2526101040301_config_temperature_offset(WE_sensorInterface_t *sensor_interface, const uint32_t *temperature_offset);
static bool gcds_2526101040301_config_altitude(WE_sensorInterface_t *sensor_interface, const uint16_t *altitude);
static bool gcds_2526101040301_config_ambient_pressure(WE_sensorInterface_t *sensor_interface, const struct sensor_value *ambient_pressure);

static int gcds_2526101040301_init(const struct device *dev);
static bool gcds_2526101040301_is_channel_valid(enum sensor_channel chan);
static bool gcds_2526101040301_is_data_ready(WE_sensorInterface_t *sensor_interface, uint16_t timeout_duration_ms, uint8_t max_step_count);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_GCDS_2526101040301_H_ */
