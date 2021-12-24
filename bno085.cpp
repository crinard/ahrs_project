#include "Arduino.h"
#include <Wire.h>

#include "bno085.h"

// Creator for BNO085 class
BNO085_IMU::BNO085_IMU(void) {}
// Destructor for BNO085 class
BNO085_IMU::~BNO085_IMU(void) {}

/**
 * @brief Initializes the BNO085 stream variable.
 **/
bool BNO085_IMU::begin(Stream *s) {
    serial_dev = s; 
    return true;
}


bool BNO085_IMU::read(imu_data_t* imu_data) {
    if (!imu_data) {
    return false;
  }

  if (!serial_dev->available()) {
    return false;
  }
  if (serial_dev->peek() != 0xAA) {
    serial_dev->read();
    return false;
  }
  // Now read all 19 bytes

  if (serial_dev->available() < 19) {
    return false;
  }
  // at this point we know there's at least 19 bytes available and the first
  if (serial_dev->read() != 0xAA) {
    // shouldn't happen baecause peek said it was 0xAA
    return false;
  }
  // make sure the next byte is the second 0xAA
  if (serial_dev->read() != 0xAA) {
    return false;
  }
  uint8_t buffer[19];
  // ok, we've got our header, read the actual data+crc
  if (!serial_dev->readBytes(buffer, 17)) {
    return false;
  };

  uint8_t sum = 0;
  // get checksum ready
  for (uint8_t i = 0; i < 16; i++) {
    sum += buffer[i];
  }
  if (sum != buffer[16]) {
    return false;
  }

  // The data comes in endian'd, this solves it so it works on all platforms
  int16_t buffer_16[6];

  for (uint8_t i = 0; i < 6; i++) {

    buffer_16[i] = (buffer[1 + (i * 2)]);
    buffer_16[i] += (buffer[1 + (i * 2) + 1] << 8);
  }
  imu_data->pitch = (float)buffer_16[1] * DEGREE_SCALE;
  imu_data->roll = (float)buffer_16[2] * DEGREE_SCALE;

  return true;
}
