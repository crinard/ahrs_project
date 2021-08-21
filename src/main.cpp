#include <Arduino.h>
#include "WiFiUdp.h"
#include "WiFiServer.h"
#include <stdio.h>

#include "drv_led.h"
#include "gdl_90.h"
#include "ins.h"

#define BLUE_LED_PIN 2
#define I2C_SPEED 115200
#define DEBUG_MSG_LEN 20
#define ONE_SECOND 1000

//Module variables
static char m_debug_msg[DEBUG_MSG_LEN] = {0};
static uint32_t m_tick = 0; //TODO: not actually tick

static INS_Data_t ins_data = {0};

void setup() {
  Serial.begin(I2C_SPEED);

  drv_led_init();
  GDL_90_init();
  ins_init();

  char msg[DEBUG_MSG_LEN] = "Debug ok: tick = ";
  for (uint8_t i = 0; i < sizeof(m_debug_msg); i++) {
      m_debug_msg[i] = msg[i];
  }
}

void loop() {
  m_tick++;
  // Debugging code
  Serial.print(m_debug_msg);
  Serial.println(m_tick);

  //Callback functions TODO: Move to better build structures
  ins_rx_cb(&ins_data);
  // GDL_90_tx_cb(&ins_data);
  blink_led(BLUE_LED);
  delay(ONE_SECOND);
}