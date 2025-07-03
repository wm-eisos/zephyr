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
	const struct sensor_value temperature_offset;
	const struct sensor_value altitude;
	const struct sensor_value ambient_pressure;

	const bool is_asc_enabled;
	const struct sensor_value co2_acs_target;
	const struct sensor_value co2_acs_initial_period;
	const struct sensor_value co2_acs_std_period;
};

/* GCDS Sensor API functions*/
static int gcds_2526101040301_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val);
static int gcds_2526101040301_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val);
static int gcds_2526101040301_sample_fetch(const struct device *dev, enum sensor_channel chan);
static int gcds_2526101040301_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val);

/* GCDS configuration functions */
static bool
gcds_2526101040301_set_config_temperature_offset(WE_sensorInterface_t *sensor_interface,
						 const struct sensor_value *temperature_offset);
static bool
gcds_2526101040301_get_config_temperature_offset(WE_sensorInterface_t *sensor_interface,
						 struct sensor_value *temperature_offset);
static bool gcds_2526101040301_set_config_altitude(WE_sensorInterface_t *sensor_interface,
						   const struct sensor_value *altitude);
static bool gcds_2526101040301_get_config_altitude(WE_sensorInterface_t *sensor_interface,
						   struct sensor_value *altitude);
static bool
gcds_2526101040301_set_config_ambient_pressure(WE_sensorInterface_t *sensor_interface,
					       const struct sensor_value *ambient_pressure);
static bool gcds_2526101040301_get_config_ambient_pressure(WE_sensorInterface_t *sensor_interface,
							   struct sensor_value *ambient_pressure);

static int gcds_2526101040301_init(const struct device *dev);
static bool gcds_2526101040301_poll_data_ready(WE_sensorInterface_t *sensor_interface,
					       uint16_t timeout_duration_ms,
					       uint8_t max_step_count);

#endif /* ZEPHYR_DRIVERS_SENSOR_WSEN_GCDS_2526101040301_H_ */
