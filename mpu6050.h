#ifndef MPU_6050_h
#define MPU_6050_h
#include "imu.h"

// Call this in the imu_init, assumes serial already initialized.
void init_mpu(void);
// Call this in the read data task.
void read_mpu_data(void);
// Call this to get data to transmit.
attitude_angles_t get_mpu_attitude(void);

#endif //MPU_6050_h