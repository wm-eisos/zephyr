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

#define TEMPERATURE_OFFSET_UNDEFINED UINT32_MAX
#define TEMPERATURE_OFFSET_MAX (uint32_t)20000

#define ALTITUDE_UNDEFINED UINT16_MAX
#define ALTITUDE_MAX (uint16_t)3000

#define AMBIENT_PRESSURE_UNDEFINED (struct sensor_value){.val1 = INT32_MAX, .val2 = INT32_MAX}
#define AMBIENT_PRESSURE_MIN (uint32_t)70000
#define AMBIENT_PRESSURE_MAX (uint32_t)120000

#define SINGLE_SHOT_TIMEOUT_DURATIION_MS (uint16_t)5000
#define SINGLE_SHOT_MAX_POLL_STEP_COUNT (uint8_t)1

#define LOW_POWER_PERIODIC_TIMEOUT_DURATIION_MS (uint16_t)30000
#define LOW_POWER_PERIODIC_MAX_POLL_STEP_COUNT (uint8_t)10

#define PERIODIC_TIMEOUT_DURATIION_MS (uint16_t)5000
#define PERIODIC_MAX_POLL_STEP_COUNT (uint8_t)10

static int gcds_2526101040301_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	if(!gcds_2526101040301_is_channel_valid(chan))
	{
		LOG_ERR("Invalid channel.");
		return -EIO;
	}

	const struct gcds_2526101040301_config *const config = dev->config;
	struct gcds_2526101040301_data *data = dev->data;

	bool is_co2_data_valid = true;

	if(config->operation_mode == gcds_2526101040301_op_mode_single_shot)
	{
		switch(chan)
		{
			case SENSOR_CHAN_CO2:
			case SENSOR_CHAN_ALL:
			{
				if(GCDS_Measure_Single_Shot(&data->sensor_interface) != WE_SUCCESS)
				{
					LOG_ERR("Failed to measure single shot.");
					return -EIO;
				}
			}
			case SENSOR_CHAN_AMBIENT_TEMP:
			case SENSOR_CHAN_HUMIDITY:
			{
				if(GCDS_Measure_Single_Shot_RHT(&data->sensor_interface) != WE_SUCCESS)
				{
					LOG_ERR("Failed to measure RHT single shot.");
					return -EIO;
				}

				is_co2_data_valid = false;
			}
			default:
				break;
		}
	}

	uint16_t timeout_duration_ms;
	uint8_t max_step_count;

	switch(config->operation_mode)
	{
		case gcds_2526101040301_op_mode_single_shot:
		{
			timeout_duration_ms = SINGLE_SHOT_TIMEOUT_DURATIION_MS;
			max_step_count = SINGLE_SHOT_MAX_POLL_STEP_COUNT;
			break;
		}
		case gcds_2526101040301_op_mode_low_power_periodic:
		{
			timeout_duration_ms = LOW_POWER_PERIODIC_TIMEOUT_DURATIION_MS;
			max_step_count = LOW_POWER_PERIODIC_MAX_POLL_STEP_COUNT;
			break;
		}
		case gcds_2526101040301_op_mode_periodic:
		{
			timeout_duration_ms = PERIODIC_TIMEOUT_DURATIION_MS;
			max_step_count = PERIODIC_MAX_POLL_STEP_COUNT;
			break;
		}
		default:
			break;
	}

	if(!gcds_2526101040301_is_data_ready(&data->sensor_interface, timeout_duration_ms, max_step_count))
	{
		return -EIO;
	}

	uint16_t co2_data;

	if(GCDS_Measure_Data(&data->sensor_interface, &co2_data, &data->temperature_data, &data->humidity_data) != WE_SUCCESS)
	{
		LOG_ERR("Failed to retrieve data from the sensor.");
		return -EIO;
	}

	if(is_co2_data_valid)
	{
		data->co2_data = co2_data;
	}

	return 0;
}

static bool gcds_2526101040301_is_channel_valid(enum sensor_channel chan)
{
	switch(chan)
	{
		case SENSOR_CHAN_CO2:
		case SENSOR_CHAN_AMBIENT_TEMP:
		case SENSOR_CHAN_HUMIDITY:
		case SENSOR_CHAN_ALL:
			return true;
		default:
			break;
	}

	return false;
}

static bool gcds_2526101040301_is_data_ready(WE_sensorInterface_t *sensor_interface, uint16_t timeout_duration_ms, uint8_t max_step_count)
{
	bool data_ready = false;
	int step_count = 0;
	uint32_t step_sleep_duration = timeout_duration_ms / max_step_count;

	while (1) {

		if(GCDS_Get_Data_Ready_Status(sensor_interface, &data_ready) != WE_SUCCESS)
		{
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

static bool gcds_2526101040301_config_temperature_offset(WE_sensorInterface_t *sensor_interface, const uint32_t *temperature_offset)
{
	if(*temperature_offset > TEMPERATURE_OFFSET_MAX)
	{
		LOG_ERR("Temperature offset is out of range.");
		return false;
	}

	return (GCDS_Set_Temperature_Offset(sensor_interface, temperature_offset) == WE_SUCCESS);
}

static bool gcds_2526101040301_config_altitude(WE_sensorInterface_t *sensor_interface, const uint16_t *altitude)
{
	if(*altitude > ALTITUDE_MAX)
	{
		LOG_ERR("Sensor altitude is out of range.");
		return false;
	}

	return (GCDS_Set_Sensor_Altitude(sensor_interface, altitude) == WE_SUCCESS);
}


static bool gcds_2526101040301_config_ambient_pressure(WE_sensorInterface_t *sensor_interface, const struct sensor_value *ambient_pressure)
{
	if(
		(ambient_pressure->val2 != 0) ||
		(ambient_pressure->val1 < AMBIENT_PRESSURE_MIN) ||
		(ambient_pressure->val1 > AMBIENT_PRESSURE_MAX)
	)
	{
		LOG_ERR("Ambient pressure is out of range.");
		return false;
	}

	return (GCDS_Set_Ambient_Pressure(sensor_interface, &ambient_pressure->val1) == WE_SUCCESS);
}

static int gcds_2526101040301_init(const struct device *dev)
{
	const struct gcds_2526101040301_config *const config = dev->config;
	struct gcds_2526101040301_data *data = dev->data;

	/* Initialize WE sensor interface */
	if(GCDS_Get_Default_Interface(&data->sensor_interface) != WE_SUCCESS)
	{
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

	if(GCDS_Reinit(&data->sensor_interface) != WE_SUCCESS)
	{
		LOG_ERR("Failed to reintialize eeprom of the sensor.");
		return -EIO;
	}

	if(config->temperature_offset != TEMPERATURE_OFFSET_UNDEFINED)
	{
		if(!gcds_2526101040301_config_temperature_offset(&data->sensor_interface, &config->temperature_offset))
		{
			LOG_ERR("Failed to set temperature offset.");
			return -EIO;
		}
	}

	if(config->altitude != ALTITUDE_UNDEFINED)
	{
		if(!gcds_2526101040301_config_altitude(&data->sensor_interface, &config->altitude))
		{
			LOG_ERR("Failed to set altitude offset.");
			return -EIO;
		}
	}

	/* TODO this needs to be checked */
	if(memcmp(&config->ambient_pressure, &AMBIENT_PRESSURE_UNDEFINED, sizeof(struct sensor_value)) != 0)
	{
		if(!gcds_2526101040301_config_ambient_pressure(&data->sensor_interface, &config->ambient_pressure))
		{
			LOG_ERR("Failed to set ambient pressure.");
			return -EIO;
		}
	}

	switch(config->operation_mode)
	{
		case gcds_2526101040301_op_mode_single_shot:
		{
			break;
		}
		case gcds_2526101040301_op_mode_low_power_periodic:
		{
			if(GCDS_Start_Low_Power_Measurement(&data->sensor_interface) != WE_SUCCESS)
			{
				LOG_ERR("Failed to start low power periodic measurement.");
				return -EIO;
			}
			break;
		}
		case gcds_2526101040301_op_mode_periodic:
		{
			if(GCDS_Start_Periodic_Measurement(&data->sensor_interface) != WE_SUCCESS)
			{
				LOG_ERR("Failed to start periodic measurement.");
				return -EIO;
			}
			break;
		}
		default:
		{
			LOG_ERR("Invalid operation mode.");
			return -EIO;
		}
	}
	
	return 0;
}

static DEVICE_API(sensor, gcds_2526101040301_driver_api) = {
	.attr_set = NULL,
	.attr_get = NULL,
	.sample_fetch = gcds_2526101040301_sample_fetch,
	.channel_get = NULL,
};

#define GCDS_2526101040301_CONFIG_OPERATION_MODE(inst) \
.operation_mode = (gcds_2526101040301_op_mode_t)(DT_INST_ENUM_IDX(inst, operation_mode)),

#define GCDS_2526101040301_CONFIG_TEMPERATURE_OFFSET(inst) \
.temperature_offset = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, temperature_offset), \
(DT_INST_PROP(inst, temperature_offset),), \
(TEMPERATURE_OFFSET_UNDEFINED,) \
)

#define GCDS_2526101040301_CONFIG_ALTITUDE(inst) \
.altitude = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, altitude), \
(DT_INST_PROP(inst, altitude),), \
(ALTITUDE_UNDEFINED,) \
)

#define GCDS_2526101040301_CONFIG_AMBIENT_PRESSURE(inst) \
.ambient_pressure = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, ambient_pressure), \
({ .val1 = DT_INST_PROP(inst, ambient_pressure)},), \
(AMBIENT_PRESSURE_UNDEFINED,) \
)


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
		GCDS_2526101040301_CONFIG_OPERATION_MODE(inst)\
		GCDS_2526101040301_CONFIG_TEMPERATURE_OFFSET(inst)\
		GCDS_2526101040301_CONFIG_ALTITUDE(inst)\
		GCDS_2526101040301_CONFIG_AMBIENT_PRESSURE(inst)\
		};         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, gcds_2526101040301_init, NULL,                          \
				     &gcds_2526101040301_data_##inst,                              \
				     &gcds_2526101040301_config_##inst, POST_KERNEL,               \
				     CONFIG_SENSOR_INIT_PRIORITY, &gcds_2526101040301_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GCDS_2526101040301_DEFINE)
