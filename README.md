# ahrs_project
### Overview:
In flight, especially flight in IMC (you can't see outside), it is desirable to know which way is up and which way is down. To this end, cockpits are requited to have [attitude indicators](https://en.wikipedia.org/wiki/Attitude_indicator), which provide a visual indication of (suprise) attitude to the pilot in almost all flight conditions. 

Two things:
- Everyone uses ForeFlight now, which can display information acquired on sensors and properly formatted over HTTP.
- [MEMS sensors](https://jewellinstruments.com/support/how-does-a-mems-sensor-work/) can be pretty good, with the right integrations and data meshing. 

### Technical Background:
- [attitude indicators](https://en.wikipedia.org/wiki/Attitude_indicator)
- [MEMS sensors](https://jewellinstruments.com/support/how-does-a-mems-sensor-work/)
- [Foreflight Communication Protocol](https://www.foreflight.com/connect/spec/)
- [ESP32](TODO)
- [EKF](TODO)
- 


### Current State:
Messages other than the 1090mHz ADS-B work (don't worry about these for now). The main problem is in doing the math/intreprations required to get accurate attitude data. It should be relatively simple I2c sensor integration, and maybe an EKF or something similar. 

I have a couple of IMUs available, tried the following:
- MPU6050, which I've integrated and seems to be okay other than drift (which could be solved with integrating GPS and an EKF)
- BNO085 (was good in a stationary enviroment, but terrible in an airplane. If I had to guess, it may have to do with the sensor fusion/parameter tuning already done to make this work better in a stationary enviroment)
- [Suggested by prof](https://www.robotshop.com/en/imu-10-dof-16g-3-axis-accelerometer-2000--s-gyromagnetometerbarometer.html) (This is incoming, could work well?)
- Not an IMU, but have this [GPS module](https://www.mouser.com/ProductDetail/SparkFun/GPS-15210?qs=Zz7%252BYVVL6bGf8coET7CrKg%3D%3D&mgh=1&gclid=Cj0KCQiA09eQBhCxARIsAAYRiymHeGObdLgaFf2PjmO-72c4x0_OGKZRrCLYyoYAehXbmmKjb3oBswoaAshFEALw_wcB) to play with. 

### Setup:
Meant to be compiled and used with Arduino IDE, with the Adafruit ESP32 Feather packages.
ESP32 + IMU to integrate backup AHRS with ForeFlight. Pretty simple Arduino setup. On ForeFlight functionality, this could use some tests and I can hook you up.
