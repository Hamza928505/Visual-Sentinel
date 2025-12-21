#!/bin/bash

# IMU Truncate
sudo truncate -s 0 /var/log/imu/ypr.jsonl

# GPS Truncate
sudo truncate -s 0 /var/log/gps/gps.jsonl

# IMU Calibration

sudo rm -f /root/.minimu9-ahrs-cal
rm -f ~/.minimu9-ahrs-cal

minimu9-ahrs-calibrate

sudo cp /home/pi01/.minimu9-ahrs-cal /root/
sudo chown root:root /root/.minimu9-ahrs-cal

echo "Pi01 IMU and GPS has been Truncated ..."
echo "Pi01 is ready."
