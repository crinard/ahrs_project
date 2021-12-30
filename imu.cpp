#include "imu.h"
#include "mpu6050.h"
#include "defines.h"

#define RPY_TO_GDL_90(x) (int16_t) (x * 10) // RPY (degrees) * 1800/180.

/****** Function Prototypes ******/

/**
 * @brief Takes a float from the sensor and converts it into the range -1800, 1800.
 **/
int16_t float_to_gdl90_range(float x);
attitude_t get_attitude(void);
/****** Tasks ******/
void task_read_rpy(void * pvParameters);

void imu_init(void) {
  init_mpu();
  xTaskCreatePinnedToCore(
    task_read_rpy
    ,  "Read from the sensor."
    ,  2048 // Stack size
    ,  NULL
    ,  3 // Priority (0-3)
    ,  NULL 
    ,  SLOW_CORE);
}

void task_read_rpy(void * pvParameters) {
    for(;;) {
      while (true) { 
        read_mpu_data();
        vTaskDelay(MS_TO_TICK(20));
      }
    }
}

attitude_t get_attitude(void) {
    attitude_angles_t imu_att = get_mpu_attitude();
    // Angles and directions set for my expected mounting here.
    return {RPY_TO_GDL_90(imu_att.pitch), RPY_TO_GDL_90(imu_att.yaw), 0x7FFF};
}
