#ifndef IMU_H
#define IMU_H

#include "Adafruit_BNO08x_RVC.h"
#include <stdint.h>

typedef struct attitude_t {
  int16_t roll;
  int16_t pitch;
} attitude_t;

void imu_init(void);
attitude_t get_attitude(void);
#endif //IMU_H
