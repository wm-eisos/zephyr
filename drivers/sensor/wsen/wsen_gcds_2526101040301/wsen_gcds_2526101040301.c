/*
 * Copyright (c) 2025 Würth Elektronik eiSos GmbH & Co. KG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT we_wsen_gcds_2526101040301

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/logging/log.h>

#include <string.h>

#include "wsen_gcds_2526101040301.h"

LOG_MODULE_REGISTER(WSEN_GCDS_2526101040301, CONFIG_SENSOR_LOG_LEVEL);

#define TEMPERATURE_OFFSET_MAX (uint32_t)20000

#define ALTITUDE_MAX (uint16_t)3000

#define AMBIENT_PRESSURE_MIN (uint32_t)70000
#define AMBIENT_PRESSURE_MAX (uint32_t)120000

#define PROPERTY_UNDEFINED                                                                         \
	(struct sensor_value)                                                                      \
	{                                                                                          \
		.val1 = INT32_MAX, .val2 = INT32_MAX                                               \
	}

#define SINGLE_SHOT_TIMEOUT_DURATIION_MS (uint16_t)0
#define SINGLE_SHOT_MAX_POLL_STEP_COUNT  (uint8_t)1

#define LOW_POWER_PERIODIC_TIMEOUT_DURATIION_MS (uint16_t)30000
#define LOW_POWER_PERIODIC_MAX_POLL_STEP_COUNT  (uint8_t)10

#define PERIODIC_TIMEOUT_DURATIION_MS (uint16_t)5000
#define PERIODIC_MAX_POLL_STEP_COUNT  (uint8_t)10

static int gcds_2526101040301_sample_fetch(const struct device *dev, enum sensor_channel chan)
{

	const struct gcds_2526101040301_config *const config = dev->config;
	struct gcds_2526101040301_data *data = dev->data;

	bool is_co2_data_valid = true;

	if (config->operation_mode == gcds_2526101040301_op_mode_single_shot) {
		switch (chan) {
		case SENSOR_CHAN_CO2:
		case SENSOR_CHAN_ALL: {
			if (GCDS_Measure_Single_Shot(&data->sensor_interface) != WE_SUCCESS) {
				LOG_ERR("Failed to measure single shot.");
				return -EIO;
			}
			break;
		}
		case SENSOR_CHAN_AMBIENT_TEMP:
		case SENSOR_CHAN_HUMIDITY: {
			if (GCDS_Measure_Single_Shot_RHT(&data->sensor_interface) != WE_SUCCESS) {
				LOG_ERR("Failed to measure RHT single shot.");
				return -EIO;
			}

			is_co2_data_valid = false;
			break;
		}
		default:
			LOG_ERR("Invalid channel.");
			return -EIO;
		}
	}

	uint16_t timeout_duration_ms;
	uint8_t max_step_count;

	switch (config->operation_mode) {
	case gcds_2526101040301_op_mode_single_shot: {
		timeout_duration_ms = SINGLE_SHOT_TIMEOUT_DURATIION_MS;
		max_step_count = SINGLE_SHOT_MAX_POLL_STEP_COUNT;
		break;
	}
	case gcds_2526101040301_op_mode_low_power_periodic: {
		timeout_duration_ms = LOW_POWER_PERIODIC_TIMEOUT_DURATIION_MS;
		max_step_count = LOW_POWER_PERIODIC_MAX_POLL_STEP_COUNT;
		break;
	}
	case gcds_2526101040301_op_mode_periodic: {
		timeout_duration_ms = PERIODIC_TIMEOUT_DURATIION_MS;
		max_step_count = PERIODIC_MAX_POLL_STEP_COUNT;
		break;
	}
	default:
		return -EIO;
	}

	if (!gcds_2526101040301_poll_data_ready(&data->sensor_interface, timeout_duration_ms,
						max_step_count)) {
		return -EIO;
	}

	uint16_t co2_data;

	if (GCDS_Measure_Data(&data->sensor_interface, &co2_data, &data->temperature_data,
			      &data->humidity_data) != WE_SUCCESS) {
		LOG_ERR("Failed to retrieve data from the sensor.");
		return -EIO;
	}

	if (is_co2_data_valid) {
		data->co2_data = co2_data;
	}

	return 0;
}

static int gcds_2526101040301_channel_get(const struct device *dev, enum sensor_channel chan,
					  struct sensor_value *val)
{
	struct gcds_2526101040301_data *data = dev->data;

	int status = 0;

	switch (chan) {
	case SENSOR_CHAN_CO2: {
		val->val1 = (int32_t)data->co2_data;
		val->val2 = (int32_t)0;
		break;
	}
	case SENSOR_CHAN_AMBIENT_TEMP: {
		status = sensor_value_from_milli(val, (int64_t)data->temperature_data);
		break;
	}
	case SENSOR_CHAN_HUMIDITY: {
		status = sensor_value_from_milli(val, (int64_t)data->humidity_data);
		break;
	}
	default:
		LOG_ERR("Invalid channel.");
		return -EIO;
	}

	return status;
}

static int gcds_2526101040301_attr_set(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, const struct sensor_value *val)
{

	struct gcds_2526101040301_data *data = dev->data;
	const struct gcds_2526101040301_config *const config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		switch (attr) {
		case SENSOR_ATTR_OFFSET: {
			if (config->operation_mode != gcds_2526101040301_op_mode_single_shot) {
				LOG_ERR("Cant change this attribute in a periodic operating mode.");
				return -EIO;
			}

			if (!gcds_2526101040301_set_config_temperature_offset(
				    &data->sensor_interface, val)) {
				LOG_ERR("Failed to set temperature offset.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_PRESS: {
		switch (attr) {
		case SENSOR_ATTR_CALIBRATION: {

			if (!gcds_2526101040301_set_config_ambient_pressure(&data->sensor_interface,
									    val)) {
				LOG_ERR("Failed to set ambient pressure.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_ALTITUDE: {
		switch (attr) {
		case SENSOR_ATTR_OFFSET: {
			if (config->operation_mode != gcds_2526101040301_op_mode_single_shot) {
				LOG_ERR("Cant change this attribute in a periodic operating mode.");
				return -EIO;
			}

			if (!gcds_2526101040301_set_config_altitude(&data->sensor_interface, val)) {
				LOG_ERR("Failed to set altitude.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	default:
		LOG_ERR("Invalid channel-attribute combination.");
		return -EIO;
	}

	return 0;
}

static int gcds_2526101040301_attr_get(const struct device *dev, enum sensor_channel chan,
				       enum sensor_attribute attr, struct sensor_value *val)
{

	struct gcds_2526101040301_data *data = dev->data;
	const struct gcds_2526101040301_config *const config = dev->config;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP: {
		switch (attr) {
		case SENSOR_ATTR_OFFSET: {
			if (config->operation_mode != gcds_2526101040301_op_mode_single_shot) {
				LOG_ERR("Cant read this attribute in a periodic operating mode.");
				return -EIO;
			}

			if (!gcds_2526101040301_get_config_temperature_offset(
				    &data->sensor_interface, val)) {
				LOG_ERR("Failed to get temperature offset.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_PRESS: {
		switch (attr) {
		case SENSOR_ATTR_CALIBRATION: {

			if (!gcds_2526101040301_get_config_ambient_pressure(&data->sensor_interface,
									    val)) {
				LOG_ERR("Failed to get ambient pressure.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	case SENSOR_CHAN_ALTITUDE: {
		switch (attr) {
		case SENSOR_ATTR_OFFSET: {
			if (config->operation_mode != gcds_2526101040301_op_mode_single_shot) {
				LOG_ERR("Cant read this attribute in a periodic operating mode.");
				return -EIO;
			}

			if (!gcds_2526101040301_get_config_altitude(&data->sensor_interface, val)) {
				LOG_ERR("Failed to get altitude.");
				return -EIO;
			}

			break;
		}
		default:
			LOG_ERR("Invalid channel-attribute combination.");
			return -EIO;
		}
		break;
	}
	default:
		LOG_ERR("Invalid channel-attribute combination.");
		return -EIO;
	}

	return 0;
}

static bool gcds_2526101040301_poll_data_ready(WE_sensorInterface_t *sensor_interface,
					       uint16_t timeout_duration_ms, uint8_t max_step_count)
{
	bool data_ready = false;
	int step_count = 0;
	uint32_t step_sleep_duration = timeout_duration_ms / max_step_count;

	while (1) {

		if (GCDS_Get_Data_Ready_Status(sensor_interface, &data_ready) != WE_SUCCESS) {
			LOG_ERR("Failed to get data ready status.");
			return false;
		}

		if (data_ready) {
			break;
		} else if (step_count >= max_step_count) {
			LOG_ERR("Sample fetch timed out.");
			return false;
		}

		step_count++;
		k_sleep(K_MSEC(step_sleep_duration));
	}

	return true;
}

static bool
gcds_2526101040301_set_config_temperature_offset(WE_sensorInterface_t *sensor_interface,
						 const struct sensor_value *temperature_offset)
{
	if ((temperature_offset->val2 != 0) ||
	    (temperature_offset->val1 > TEMPERATURE_OFFSET_MAX)) {
		LOG_ERR("Temperature offset is out of range.");
		return false;
	}

	return (GCDS_Set_Temperature_Offset(sensor_interface,
					    (const uint32_t *)&temperature_offset->val1) ==
		WE_SUCCESS);
}

static bool
gcds_2526101040301_get_config_temperature_offset(WE_sensorInterface_t *sensor_interface,
						 struct sensor_value *temperature_offset)
{
	if (temperature_offset == NULL) {
		return false;
	}

	if (GCDS_Get_Temperature_Offset(sensor_interface, (uint32_t *)&temperature_offset->val1) !=
	    WE_SUCCESS) {
		return false;
	}

	temperature_offset->val2 = 0;

	return true;
}

static bool gcds_2526101040301_set_config_altitude(WE_sensorInterface_t *sensor_interface,
						   const struct sensor_value *altitude)
{
	if ((altitude->val2 != 0) || (altitude->val1 > ALTITUDE_MAX)) {
		LOG_ERR("Sensor altitude is out of range.");
		return false;
	}

	return (GCDS_Set_Sensor_Altitude(sensor_interface, (const uint16_t *)&altitude->val1) ==
		WE_SUCCESS);
}

static bool gcds_2526101040301_get_config_altitude(WE_sensorInterface_t *sensor_interface,
						   struct sensor_value *altitude)
{
	if (altitude == NULL) {
		return false;
	}

	if (GCDS_Get_Sensor_Altitude(sensor_interface, (uint16_t *)&altitude->val1) != WE_SUCCESS) {
		return false;
	}

	altitude->val2 = 0;

	return true;
}

static bool
gcds_2526101040301_set_config_ambient_pressure(WE_sensorInterface_t *sensor_interface,
					       const struct sensor_value *ambient_pressure)
{
	if ((ambient_pressure->val2 != 0) || (ambient_pressure->val1 < AMBIENT_PRESSURE_MIN) ||
	    (ambient_pressure->val1 > AMBIENT_PRESSURE_MAX)) {
		LOG_ERR("Ambient pressure is out of range.");
		return false;
	}

	return (GCDS_Set_Ambient_Pressure(sensor_interface,
					  (const uint32_t *)&ambient_pressure->val1) == WE_SUCCESS);
}

static bool gcds_2526101040301_get_config_ambient_pressure(WE_sensorInterface_t *sensor_interface,
							   struct sensor_value *ambient_pressure)
{
	if (ambient_pressure == NULL) {
		return false;
	}

	if (GCDS_Get_Ambient_Pressure(sensor_interface, (uint32_t *)&ambient_pressure->val1) !=
	    WE_SUCCESS) {
		return false;
	}

	ambient_pressure->val2 = 0;

	return true;
}

static int gcds_2526101040301_init(const struct device *dev)
{
	const struct gcds_2526101040301_config *const config = dev->config;
	struct gcds_2526101040301_data *data = dev->data;

	/* Initialize WE sensor interface */
	if (GCDS_Get_Default_Interface(&data->sensor_interface) != WE_SUCCESS) {
		return -EIO;
	}

	data->sensor_interface.interfaceType = WE_i2c;

	if (!i2c_is_ready_dt(&config->bus_cfg.i2c)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	data->sensor_interface.handle = (void *)&config->bus_cfg.i2c;

	/* First communication test - check device ID */
	if (GCDS_Init(&data->sensor_interface) != WE_SUCCESS) {
		LOG_ERR("Failed to read device ID.");
		return -EIO;
	}

	if (GCDS_Reinit(&data->sensor_interface) != WE_SUCCESS) {
		LOG_ERR("Failed to reintialize eeprom of the sensor.");
		return -EIO;
	}

	if (memcmp(&config->temperature_offset, &PROPERTY_UNDEFINED, sizeof(struct sensor_value)) !=
	    0) {
		if (!gcds_2526101040301_set_config_temperature_offset(
			    &data->sensor_interface, &config->temperature_offset)) {
			LOG_ERR("Failed to set temperature offset.");
			return -EIO;
		}
	}

	if (memcmp(&config->altitude, &PROPERTY_UNDEFINED, sizeof(struct sensor_value)) != 0) {
		if (!gcds_2526101040301_set_config_altitude(&data->sensor_interface,
							    &config->altitude)) {
			LOG_ERR("Failed to set altitude.");
			return -EIO;
		}
	}

	if (memcmp(&config->ambient_pressure, &PROPERTY_UNDEFINED, sizeof(struct sensor_value)) !=
	    0) {
		if (!gcds_2526101040301_set_config_ambient_pressure(&data->sensor_interface,
								    &config->ambient_pressure)) {
			LOG_ERR("Failed to set ambient pressure.");
			return -EIO;
		}
	}

	if (GCDS_Set_Auto_Self_Calib_Enabled(&data->sensor_interface,
					     (bool *)&config->is_asc_enabled) != WE_SUCCESS) {
		LOG_ERR("Failed to set auto self calibration state.");
		return -EIO;
	}

	if (memcmp(&config->co2_acs_target, &PROPERTY_UNDEFINED, sizeof(struct sensor_value)) !=
	    0) {
		if (GCDS_Set_Auto_Self_Calib_Target(
			    &data->sensor_interface,
			    (const uint16_t *)&(config->co2_acs_target.val1)) != WE_SUCCESS) {
			LOG_ERR("Failed to set auto self calibration target.");
			return -EIO;
		}
	}

	if (memcmp(&config->co2_acs_initial_period, &PROPERTY_UNDEFINED,
		   sizeof(struct sensor_value)) != 0) {
		if (config->co2_acs_initial_period.val1 % 4 != 0) {
			LOG_ERR("Invalid auto self calibration initial period value.");
			return -EIO;
		}

		if (GCDS_Set_Auto_Self_Calib_Init_Period(
			    &data->sensor_interface,
			    (const uint16_t *)&(config->co2_acs_initial_period.val1)) !=
		    WE_SUCCESS) {
			LOG_ERR("Failed to set auto self calibration initial period.");
			return -EIO;
		}
	}

	if (memcmp(&config->co2_acs_std_period, &PROPERTY_UNDEFINED, sizeof(struct sensor_value)) !=
	    0) {
		if (config->co2_acs_std_period.val1 % 4 != 0) {
			LOG_ERR("Invalid auto self calibration standard period value.");
			return -EIO;
		}

		if (GCDS_Set_Auto_Self_Calib_Std_Period(
			    &data->sensor_interface,
			    (const uint16_t *)&(config->co2_acs_std_period.val1)) != WE_SUCCESS) {
			LOG_ERR("Failed to set auto self calibration standard period.");
			return -EIO;
		}
	}

	switch (config->operation_mode) {
	case gcds_2526101040301_op_mode_single_shot: {
		break;
	}
	case gcds_2526101040301_op_mode_low_power_periodic: {
		if (GCDS_Start_Low_Power_Measurement(&data->sensor_interface) != WE_SUCCESS) {
			LOG_ERR("Failed to start low power periodic measurement.");
			return -EIO;
		}
		break;
	}
	case gcds_2526101040301_op_mode_periodic: {
		if (GCDS_Start_Periodic_Measurement(&data->sensor_interface) != WE_SUCCESS) {
			LOG_ERR("Failed to start periodic measurement.");
			return -EIO;
		}
		break;
	}
	default: {
		LOG_ERR("Invalid operation mode.");
		return -EIO;
	}
	}

	return 0;
}

static DEVICE_API(sensor, gcds_2526101040301_driver_api) = {
	.attr_set = gcds_2526101040301_attr_set,
	.attr_get = gcds_2526101040301_attr_get,
	.sample_fetch = gcds_2526101040301_sample_fetch,
	.channel_get = gcds_2526101040301_channel_get,
};

#define GCDS_2526101040301_CONFIG_OPERATION_MODE(inst)                                             \
	.operation_mode = (gcds_2526101040301_op_mode_t)(DT_INST_ENUM_IDX(inst, operation_mode)),

#define GCDS_2526101040301_CONFIG_TEMPERATURE_OFFSET(inst)                                         \
	.temperature_offset = COND_CODE_1(                                                    \
        DT_INST_NODE_HAS_PROP(inst, temperature_offset),                                  \
        ({ .val1 = DT_INST_PROP(inst, temperature_offset) },),                            \
        (PROPERTY_UNDEFINED,)                                                             \
    )

#define GCDS_2526101040301_CONFIG_ALTITUDE(inst)                                                   \
	.altitude = COND_CODE_1(                                                              \
        DT_INST_NODE_HAS_PROP(inst, altitude),                                            \
        ({ .val1 = DT_INST_PROP(inst, altitude) },),                                      \
        (PROPERTY_UNDEFINED,)                                                             \
    )

#define GCDS_2526101040301_CONFIG_AMBIENT_PRESSURE(inst)                                           \
	.ambient_pressure = COND_CODE_1(                                                     \
        DT_INST_NODE_HAS_PROP(inst, ambient_pressure),                                   \
        ({ .val1 = DT_INST_PROP(inst, ambient_pressure) },),                             \
        (PROPERTY_UNDEFINED,)                                                            \
    )

#define GCDS_2526101040301_CONFIG_ACS_STATE(inst)                                                  \
	.is_asc_enabled = DT_INST_PROP(inst, automatic_self_calibration),

#define GCDS_2526101040301_CONFIG_ACS_TARGET(inst)                                                 \
	.co2_acs_target = COND_CODE_1(                                                       \
        DT_INST_NODE_HAS_PROP(inst, automatic_self_calibration_target),                  \
        ({ .val1 = DT_INST_PROP(inst, automatic_self_calibration_target) },),            \
        (PROPERTY_UNDEFINED,)                                                            \
    )

#define GCDS_2526101040301_CONFIG_ACS_INIT_PERIOD(inst)                                            \
	.co2_acs_initial_period = COND_CODE_1(                                                 \
        DT_INST_NODE_HAS_PROP(inst, automatic_self_calibration_initial_period),            \
        ({ .val1 = DT_INST_PROP(inst, automatic_self_calibration_initial_period) },),      \
        (PROPERTY_UNDEFINED,)                                                              \
    )

#define GCDS_2526101040301_CONFIG_ACS_STD_PERIOD(inst)                                             \
	.co2_acs_std_period = COND_CODE_1(                                                   \
        DT_INST_NODE_HAS_PROP(inst, automatic_self_calibration_standard_period),         \
        ({ .val1 = DT_INST_PROP(inst, automatic_self_calibration_standard_period) },),   \
        (PROPERTY_UNDEFINED,)                                                            \
    )

#define GCDS_2526101040301_CONFIG(inst)                                                            \
	GCDS_2526101040301_CONFIG_OPERATION_MODE(inst)                                             \
	GCDS_2526101040301_CONFIG_TEMPERATURE_OFFSET(inst)                                         \
	GCDS_2526101040301_CONFIG_ALTITUDE(inst)                                                   \
	GCDS_2526101040301_CONFIG_AMBIENT_PRESSURE(inst)                                           \
	GCDS_2526101040301_CONFIG_ACS_STATE(inst)                                                  \
	GCDS_2526101040301_CONFIG_ACS_TARGET(inst)                                                 \
	GCDS_2526101040301_CONFIG_ACS_INIT_PERIOD(inst)                                            \
	GCDS_2526101040301_CONFIG_ACS_STD_PERIOD(inst)

/*
 * Main instantiation macro.
 */
#define GCDS_2526101040301_DEFINE(inst)                                                            \
	static struct gcds_2526101040301_data gcds_2526101040301_data_##inst;                      \
	static const struct gcds_2526101040301_config gcds_2526101040301_config_##inst = {         \
		.bus_cfg =                                                                         \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
			},                                                                         \
		GCDS_2526101040301_CONFIG(inst)};                                                  \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, gcds_2526101040301_init, NULL,                          \
				     &gcds_2526101040301_data_##inst,                              \
				     &gcds_2526101040301_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &gcds_2526101040301_driver_api)

DT_INST_FOREACH_STATUS_OKAY(GCDS_2526101040301_DEFINE)
