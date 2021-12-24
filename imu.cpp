#include "imu.h"
#include <Arduino.h>
#include <Adafruit_BNO08x.h>

#define MS_TO_TICK(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define FAST_CORE 0
#define SLOW_CORE 1
#define BNO08X_RESET -1

/****** Module Variables ******/
static Adafruit_BNO08x  bno08x(BNO08X_RESET);
static sh2_SensorValue_t sensorValue;
static ahrs_data_t m_ff_attitude;
static void set_reports(void);
static inline void quaternion_to_attitude(sh2_GyroIntegratedRV_t* imu_data, ahrs_data_t* data);
/****** Function Prototypes ******/

/**
 * @brief Takes a float from the sensor and converts it into the range -1800, 1800.
 **/
static inline int16_t float_to_gdl90_range(float x);
ahrs_data_t get_attitude(void);
/****** Tasks ******/
void task_read_rpy(void * pvParameters);

void imu_init(void) {
  while (!bno08x.begin_UART(&Serial1)) {
    Serial.println("BNO085 not found\n");
    delay(500);
  }

  set_reports();

  xTaskCreatePinnedToCore(
    task_read_rpy
    ,  "Read from the sensor."
    ,  2048 // Stack size
    ,  NULL
    ,  2 // Priority (0-3)
    ,  NULL 
    ,  SLOW_CORE);
}

void task_read_rpy(void * pvParameters) {
  for(;;) {
    
    if (bno08x.wasReset()) {
      Serial.print("sensor was reset ");
      set_reports();
    }

    while (bno08x.getSensorEvent(&sensorValue)) {
      //Process as many as the task scheduler allows for.
      //TODO: This could be better implemented.
      switch (sensorValue.sensorId) {
          case SH2_GYRO_INTEGRATED_RV:
            quaternion_to_attitude(&sensorValue.un.gyroIntegratedRV, &m_ff_attitude);
            break;
          case SH2_GEOMAGNETIC_ROTATION_VECTOR:
            Serial.print("Geo-Magnetic Rotation Vector - r: ");
            Serial.print(sensorValue.un.geoMagRotationVector.real);
            Serial.print(" i: ");
            Serial.print(sensorValue.un.geoMagRotationVector.i);
            Serial.print(" j: ");
            Serial.print(sensorValue.un.geoMagRotationVector.j);
            Serial.print(" k: ");
            Serial.println(sensorValue.un.geoMagRotationVector.k);
            break;
          default:
            break;
      }
    }
    vTaskDelay(MS_TO_TICK(100));
  }
}

static void set_reports(void) {
  if (!bno08x.enableReport(SH2_GYRO_INTEGRATED_RV, 200)) {
    Serial.println("Could not enable gyro");
  }
  if (!bno08x.enableReport(SH2_GEOMAGNETIC_ROTATION_VECTOR)) {
    Serial.println("Could not enable geomag");
  } 
}

static inline void quaternion_to_attitude(sh2_GyroIntegratedRV_t* imu_data, ahrs_data_t* data) {
  float qr = imu_data->real;
  float qi = imu_data->i;
  float qj = imu_data->j;
  float qk = imu_data->k;
  float sqr = qr * qr;
  float sqi = qi * qr;
  float sqj = sq(qj);
  float sqk = sq(qk);
  data->pitch = float_to_gdl90_range(
    asin(-2.0 * (qi * qk - qj * qr) / (sqi + sqj + sqk + sqr)) * RAD_TO_DEG);
  data->roll = float_to_gdl90_range(
    atan2(2.0 * (qj * qk + qi * qr), (-sqi - sqj + sqk + sqr)) * RAD_TO_DEG);
}

/**
 * @brief Takes a float from the sensor and converts it into the range -1800, 1800.
 **/
static inline int16_t float_to_gdl90_range(float x) {
  double dbl = (double) x;
  dbl = dbl * (1800 / 180);
  return (int16_t) dbl;
}

ahrs_data_t get_attitude(void) {
  return m_ff_attitude;
}
