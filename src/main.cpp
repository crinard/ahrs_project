#include <Arduino.h>
#include <stdio.h>

#include "drv_led.h"
#include "gdl_90.h"
#include "ins.h"

#define I2C_SPEED 115200
#define ONE_SECOND 1000

//Module variables
static INS_Data_t m_ins_data = {0};

void setup() {
  Serial.begin(I2C_SPEED);
  drv_led_init();
  GDL_90_init();
}

void loop() {
  GDL_90_tx_cb(&m_ins_data);
  delay(200);
  blink_led(BLUE_LED);
}
