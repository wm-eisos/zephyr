.. _wsen-itds-2533020201601:

WSEN-ITDS-2533020201601: 3-axis acceleration sensor
###################################################

Overview
********
 This sample periodically measures acceleration in 3-axis and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-ITDS sensor controlled using the I2C interface.

References
**********

 - WSEN-ITDS: https://www.we-online.com/catalog/en/manual/2533020201601

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-ITDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_itds_2533020201601/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 27.430000

  I: Acceleration (g): x=-0.989940, y=-0.035995, z=-0.023963
