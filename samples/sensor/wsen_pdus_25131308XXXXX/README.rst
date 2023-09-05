.. _wsen-pdus-25131308XXXXX:

WSEN-PDUS-25131308XXXXX: Differential pressure sensor
#####################################################

Overview
********
 This sample periodically measures differential pressure and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-PDUS sensor controlled using the I2C interface.

References
**********

 - WSEN-PDUS: https://www.we-online.com/en/components/products/WSEN-PDUS

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-ITDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_pdus_25131308XXXXX/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 21.406992

  I: Pressure (kPa): -0.051280
