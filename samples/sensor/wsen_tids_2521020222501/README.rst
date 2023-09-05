.. _wsen-tids-2521020222501:

WSEN-TIDS-2521020222501: Temperature Sensor
###########################################

Overview
********
 This sample periodically measures ambient temperature for
 50 sec in the interval of 500msec in polling mode. It also sets up triggers for high and low temperature thresholds.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-TIDS sensor controlled using the I2C interface.

References
**********

 - WSEN-TIDS: https://www.we-online.com/catalog/en/manual/2521020222501

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-ITDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_tids_2521020222501/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 26.020000

  I: High temperature threshold
