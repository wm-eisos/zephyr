.. _wsen-hids-2525020210001:

WSEN-HIDS-2525020210001: Humidity sensor
########################################

Overview
********
 This sample periodically measures humidity and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 It also sets up the trigger for data ready if triggers are enabled for the sensor.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-HIDS sensor controlled using the I2C interface.

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-HIDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_hids_2525020210001/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 27.210000

  I: Humidity (%): 44.230000
