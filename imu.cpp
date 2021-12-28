#include "imu.h"
#include "Adafruit_BNO08x_RVC.h"
#define ms_to_tick(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define FAST_CORE 0
#define SLOW_CORE 1

/****** Module Variables ******/
Adafruit_BNO08x_RVC rvc = Adafruit_BNO08x_RVC();
static BNO08x_RVC_Data m_attitude;
static attitude_t m_ff_attitude;

/****** Function Prototypes ******/

/**
 * @brief Takes a float from the sensor and converts it into the range -1800, 1800.
 **/
int16_t float_to_gdl90_range(float x);
attitude_t get_attitude(void);
/****** Tasks ******/
void task_read_rpy(void * pvParameters);

void imu_init(void) {
  Serial1.begin(115200); // This is the baud rate specified by the datasheet for the IMU chip.
  while (!Serial1) delay(20);
  while (!rvc.begin(&Serial1)) { // connect to the sensor over hardware serial
    Serial.println("Could not find BNO08x!");
    delay(10);
  }
  Serial.println("BNO08x found!");
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
    // The example script was polling TODO: Setup interrupts.
    for(;;) {
      while(rvc.read(&m_attitude)) {
        // We have the most recent buffer, save into our struct and delay. 
        // TODO: make this atomic.
        m_ff_attitude.roll = float_to_gdl90_range(m_attitude.pitch);
        m_ff_attitude.pitch = float_to_gdl90_range(m_attitude.roll);
      }
      vTaskDelay(ms_to_tick(50));
    }
}

/**
 * @brief Takes a float from the sensor and converts it into the range -1800, 1800.
 **/
int16_t float_to_gdl90_range(float x) {
    double dbl = (double) x;
    dbl = dbl * 10;
    return (int16_t) dbl;
}

attitude_t get_attitude(void) {
    return m_ff_attitude;
}
