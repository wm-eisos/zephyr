.. _wsen-hids-2525020210002:

WSEN-HIDS-2525020210002: Humidity sensor
########################################

Overview
********
 This sample periodically measures humidity and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-HIDS sensor controlled using the I2C interface.

References
**********

 - WSEN-HIDS: https://www.we-online.com/catalog/en/manual/2525020210002

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-HIDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_hids_2525020210002/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 26.122999

  I: Humidity (%): 60.550999
