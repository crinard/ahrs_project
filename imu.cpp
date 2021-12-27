#include "imu.h"
#include <Arduino.h>
#include <Adafruit_BNO08x.h>

#define MS_TO_TICK(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define US_TO_S(x) x * 0.000001
#define FAST_CORE 0
#define SLOW_CORE 1
#define BNO08X_RESET -1
#define ADC_TO_ACCEL 0.0245 // Constant to convert from BNO085 units to m/s^2 (GDL90 range, -1800-1800) for accleration.
#define TO_GDL_90_RANGE(x) (uint16_t) (x * 10.0f); // Converts from deg to (GDL90 range, -1800-1800).

/****** Module Variables ******/

static Adafruit_BNO08x  bno08x(BNO08X_RESET);
static sh2_SensorValue_t sensorValue;
static ahrs_data_t m_ff_attitude;
static double m_x, m_y, m_z; //Pitch and roll, in degrees.
static uint32_t gyro_rx_time, accel_rx_time; // Timestamps for the last recieved gyro and accel data.

/****** Function Prototypes ******/

static void set_reports(void);
static inline void read_uncal_gyro(sh2_GyroscopeUncalibrated_t* raw_gyro, ahrs_data_t* data);
static inline void read_raw_mag(sh2_MagneticField_t* raw_mag, ahrs_data_t* data);
static inline void read_raw_accel(sh2_RawAccelerometer_t* raw_accel, ahrs_data_t* data);

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

    for (size_t i = 0; i < 40; i++) { //Read 40 each time.
      bno08x.getSensorEvent(&sensorValue);
      switch (sensorValue.sensorId) {
          case SH2_GYROSCOPE_UNCALIBRATED:
            read_uncal_gyro(&sensorValue.un.gyroscopeUncal, &m_ff_attitude);
            break;
          case SH2_RAW_MAGNETOMETER:
//            read_raw_mag(&sensorValue.un.magneticField, &m_ff_attitude);
            break;
          case SH2_RAW_ACCELEROMETER:
//            read_raw_accel(&sensorValue.un.rawAccelerometer, &m_ff_attitude);
            break;
          case SH2_ACCELEROMETER:
//            Serial.printf("Acc: %f %f %f\n", sensorValue.un.accelerometer.x, sensorValue.un.accelerometer.y, sensorValue.un.accelerometer.z);
            break;
          case SH2_GYROSCOPE_CALIBRATED:
//            Serial.printf("Cald: %f, %f, %f\n",sensorValue.un.gyroscope.x, sensorValue.un.gyroscope.y, sensorValue.un.gyroscope.z);
            break; 
          default:
            break;
      }
    }
    vTaskDelay(MS_TO_TICK(100));
  }
}

static void set_reports(void) {
  if (!bno08x.enableReport(SH2_ACCELEROMETER)) {
    Serial.println("Could not enable accelerometer");
  }
  if (!bno08x.enableReport(SH2_GYROSCOPE_CALIBRATED)) {
    Serial.println("Could not enable gyroscope");
  }
  if (!bno08x.enableReport(SH2_MAGNETIC_FIELD_CALIBRATED)) {
    Serial.println("Could not enable magnetic field calibrated");
  }
  if (!bno08x.enableReport(SH2_RAW_ACCELEROMETER)) {
    Serial.println("Could not enable raw accelerometer");
  }
  if (!bno08x.enableReport(SH2_GYROSCOPE_UNCALIBRATED)) {
    Serial.println("Could not enable raw gyroscope");
  }
  if (!bno08x.enableReport(SH2_RAW_MAGNETOMETER)) {
    Serial.println("Could not enable raw magnetometer");
  }
}

static inline void read_uncal_gyro(sh2_GyroscopeUncalibrated_t* raw_gyro, ahrs_data_t* data) {
  uint32_t delta_t = raw_gyro->timestamp - gyro_rx_time;
  double x = ((double)raw_gyro->x) * US_TO_S((double)delta_t);
  double y = ((double)raw_gyro->y) * US_TO_S((double)delta_t);
  double z = ((double)raw_gyro->z) * US_TO_S((double)delta_t);
  m_x += x;
  m_y += y;
  m_z += z;
  gyro_rx_time = raw_gyro->timestamp;
  Serial.printf("x = %f, y = %f, z = %f\n", m_x, m_y, m_z);
}

static inline void read_raw_accel(sh2_GyroscopeUncalibrated_t* raw_accel, ahrs_data_t* data) {
  double x = ((double)raw_accel->x) * ADC_TO_ACCEL;
  double y = ((double)raw_accel->y) * ADC_TO_ACCEL;
  double z = ((double)raw_accel->z) * ADC_TO_ACCEL;
}

static inline void read_raw_mag(sh2_MagneticField_t* raw_mag, ahrs_data_t* data) {
//  TODO: Handle other two (edge) cases in Honeywell AN203 Compass Heading Using Magnetometers.
//  data->heading = (raw_mag->y > 0) ? 
//                 TO_(90 - atan(raw_mag->x/raw_mag->y) * RAD_TO_DEG) : 
//                 float_to_gdl90_range(270 - atan(raw_mag->x/raw_mag->y) * RAD_TO_DEG);
  Serial.printf("Raw magnetometer: x = %i, y = %i, z = %i\n", raw_mag->x, raw_mag->y, raw_mag->z);
}

ahrs_data_t get_attitude(void) {
  return m_ff_attitude;
}
