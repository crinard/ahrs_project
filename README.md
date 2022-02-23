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
Messages other than the 1090mHz ADS-B work (don't worry about these for now). The main problem is in doing the math/intreprations required to get accurate attitude data. It should be relatively simple I2c sensor integration. 

### Setup:
Meant to be compiled and used with Arduino IDE, with the Adafruit ESP32 Feather packages.
ESP32 + IMU to integrate backup AHRS with ForeFlight. 
