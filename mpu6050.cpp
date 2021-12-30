#include "mpu6050.h"
#include "imu.h"
#include <Wire.h>
#include "Arduino.h"

/** @cite: https://github.com/gabriel-milan/TinyMPU6050 for example code on MPU6050 interface. Was greatly helpful**/

/*
 *	MPU 6050 Stuff
 */
#define GYRO_TRANSFORMATION_NUMBER      0.01525878906 // (1 / 65.536) precalculated
#define MPU_6050_ADDR           0x68
#define MPU6050_GYRO_XOUT_H		0x43	// Initial register address for gyro data
// Config regs
#define MPU6050_SMPLRT_DIV  	0x19
#define MPU6050_CONFIG      	0x1A
#define MPU6050_GYRO_CONFIG 	0x1B
#define MPU6050_ACCEL_CONFIG	0x1C
#define MPU6050_PWR_MGMT_1      0x6B

#define RPY_TO_GDL_90(x) (int16_t) (x * 10) // RPY (degrees) * 1800/180.
static int16_t initial_x, initial_y, initial_z;
static int16_t raw_x, raw_y, raw_z;
static float roll, pitch, yaw;
static unsigned long intervalStart;
static void RegisterWrite(byte registerAddress, byte data);
static TwoWire *wire;

void init_mpu(void) {
  wire = &Wire;

  // Initialization
  wire->begin();
	// Setting sample rate divider
	RegisterWrite(MPU6050_SMPLRT_DIV, 0x00);

	// Setting frame synchronization and the digital low-pass filter
	RegisterWrite(MPU6050_CONFIG, 0x00);

	// Setting gyro self-test and full scale range
	RegisterWrite(MPU6050_GYRO_CONFIG, 0x08);

	// Setting accelerometer self-test and full scale range
	RegisterWrite(MPU6050_ACCEL_CONFIG, 0x00);

	// Waking up MPU6050
	RegisterWrite(MPU6050_PWR_MGMT_1, 0x01);

	// Setting angles to zero
  roll = 0;
  pitch = 0;

  // Get the initial value TODO: Maybe to a 5s average here?
  wire->beginTransmission(MPU_6050_ADDR);

  // Accessing gyro data registers
  wire->write(MPU6050_GYRO_XOUT_H);
  wire->endTransmission(false);

  // Requesting gyro data
  wire->requestFrom(MPU_6050_ADDR, 6, (int) true);

  // Storing raw gyro data
  initial_x = wire->read() << 8;
  initial_x |= wire->read();

  initial_y = wire->read() << 8;
  initial_y |= wire->read();

  initial_z = wire->read() << 8;
  initial_z |= wire->read();
}

void read_mpu_data(void) {
	wire->beginTransmission(MPU_6050_ADDR);

	// Accessing gyro data registers
	wire->write(MPU6050_GYRO_XOUT_H);
	wire->endTransmission(false);

	// Requesting gyro data
	wire->requestFrom(MPU_6050_ADDR, 6, (int) true);

	// Storing raw gyro data
	raw_x = wire->read() << 8;
	raw_x |= wire->read();

	raw_y = wire->read() << 8;
	raw_y |= wire->read();

	raw_z = wire->read() << 8;
	raw_z |= wire->read();

  float dt = (millis() - intervalStart) * 0.001;
  // Computing gyro angles
	roll = (roll + ((float)raw_x - initial_x) * GYRO_TRANSFORMATION_NUMBER * dt);
	pitch = (pitch + ((float)raw_y - initial_y) * GYRO_TRANSFORMATION_NUMBER * dt);
  yaw = (yaw + ((float)raw_z - initial_z) * GYRO_TRANSFORMATION_NUMBER * dt);
  intervalStart = millis();
}

static void RegisterWrite(byte registerAddress, byte data) {
  // Starting transmission for MPU6050
  wire->beginTransmission(MPU_6050_ADDR);

  // Accessing register
  wire->write(registerAddress);

  // Writing data
  wire->write(data);

  // Closing transmission
  wire->endTransmission();
}

attitude_angles_t get_mpu_attitude(void) {
    attitude_angles_t ret = {roll, pitch, yaw, 0.0f};
    return ret;
}
