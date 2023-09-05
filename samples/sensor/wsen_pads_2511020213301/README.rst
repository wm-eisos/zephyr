.. _wsen-pads-2511020213301:

WSEN-PADS-2511020213301: Absolute pressure sensor
#################################################

Overview
********
 This sample periodically measures absolute and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-PADS sensor controlled using the I2C interface.

References
**********

 - WSEN-PADS: https://www.we-online.com/catalog/en/manual/2511020213301

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-PADS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_pads_2511020213301/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 32.060000

  I: Pressure (kPa): 100.181000
