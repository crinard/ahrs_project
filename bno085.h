#ifndef BNO085_H
#define BNO085_H 

/** @cite https://github.com/adafruit/Adafruit_BNO08x_RVC **/

#include "Arduino.h"

#define MILLI_G_TO_MS2 0.0098067 ///< Scalar to convert milli-gs to m/s^2
#define DEGREE_SCALE 0.01        ///< To convert the degree values

typedef struct imu_data_t {
  float roll;
  float pitch;
  float heading;
} imu_data_t;

class BNO085_IMU {
  public:
  BNO085_IMU();
  ~BNO085_IMU();

  bool begin(Stream *s);
  bool read(imu_data_t *imu_data);

private:
  Stream *serial_dev;
};

#endif