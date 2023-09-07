.. _wsen-isds-2536030320001:

WSEN-ISDS: 6 Axis IMU (Inertial Measurement Unit)
#################################################

Overview
********
 This sample periodically measures acceleration in 3-axis, angular rate in 3-axis and ambient temperature for
 50 sec in the interval of 500msec in polling mode.
 The result is displayed on the console.

Requirements
************

 This sample uses the WSEN-ISDS sensor controlled using the I2C interface..

References
**********

 - WSEN-ISDS: https://www.we-online.com/catalog/en/manual/2536030320001

Building and Running
********************

 This project outputs sensor data to the console. It requires a WSEN-ISDS
 sensor.

 .. zephyr-app-commands::
    :app: samples/sensor/wsen_isds_2536030320001/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

  I: Temperature (Celsius): 23.709999

  I: Acceleration (g): x=0.003976, y=0.959960, z=0.066995

  I: Angular Rate (deg/s): x=1.375090, y=-1.833460, z=-0.802140
