#ifndef IMU_H
#define IMU_H

#include <stdint.h>

typedef struct ahrs_data_t {
  int16_t roll;
  int16_t pitch;
  int16_t heading;
} ahrs_data_t;

void imu_init(void);
ahrs_data_t get_attitude(void);
#endif //IMU_H
