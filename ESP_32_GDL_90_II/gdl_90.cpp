#include "gdl_90.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>

#define ms_to_tick(x) x/portTICK_PERIOD_MS //Arduino default ticks.


/****** Defines ******/
#define NET_NAME "backup_ahrs"
#define RX_PORT 63093
#define TX_PORT 4000

/****** Module Variables ******/
static uint16_t Crc16Table[256] = {0};
static WiFiUDP udp;
static IPAddress m_foreflight_ip = IPAddress(192,168,255,255); //Default to very visible for debugging

/****** Tasks ******/
static void TaskGetFFIP (void *pvParameters );
static void TaskSendHeartbeatMsg( void *pvParameters );
//static void TaskSendAHRS( void *pvParameters );

/****** Function prototypes ******/

/**
 * @brief Initializes crc table for Flag byte and CRC.
 * @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
static void crc_init(void);
/**
 * @brief Adds Flag byte and CRC.
 * @param msg The beginning of the message to be sent.
 * @param length The length of the message buffer.
 * @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
static void crc_inject(unsigned char *msg, uint32_t length);

/**
 * @brief Initializes all components needed for GDL90 w/ foreflight.
 **/
void gdl_90_init(void) {
    // WiFi AP setup, independent of tasks.
  WiFi.softAP(NET_NAME);
  udp.begin(RX_PORT);
  crc_init();
  
  xTaskCreatePinnedToCore(
    TaskSendHeartbeatMsg
    ,  "Send Heartbeat messages to Foreflight."
    ,  4096 // Stack size
    ,  NULL
    ,  1 // Priority (0-3)
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

//  xTaskCreatePinnedToCore(
//    TaskSendAHRS
//    ,  "Send AHRS messages to Foreflight."
//    ,  4096  // Stack size 
//    ,  NULL
//    ,  3  //Priority (0-3)
//    ,  NULL 
//    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "Listen for Foreflight JSON messages."
    ,  4096  // Stack size 
    ,  NULL
    ,  1  //Priority (0-3)
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
  
}

static void crc_init(void) {
    uint16_t i, bitctr, crc;
    for (i = 0; i < 256; i++)
    {
        crc = (i << 8);
        for (bitctr = 0; bitctr < 8; bitctr++)
        {
            crc = (crc << 1) ^ ((crc & 0x8000) ? 0x1021 : 0);
        }
        Crc16Table[i] = crc;
    }
}

static void crc_inject(unsigned char *msg, uint32_t length) {
    uint32_t i;
    uint16_t crc = 0;
    // Return – CRC of the block
    // i – Starting address of message
    // i – Length of message
    for (i = 1; i < length - 3; i++) {
        crc = Crc16Table[crc >> 8] ^ (crc << 8) ^ msg[i];
    }
    //Now we have the crc, put it in the bytes third from last and second from last
    msg[length - 3] = (uint8_t)(crc & 0x00FF);
    msg[length - 2] = (uint8_t)((crc & 0xFF00) >> 8);
}

static void TaskGetFFIP (void *pvParameters ) {
  // Variable setup.
  static char rx_buf[255]; //buffer to hold incoming packet
  for (;;) {
    
    // if there's data available, read a packet
    int packetSize = udp.parsePacket();
    if (packetSize) {
      Serial.print("From ");
      m_foreflight_ip = udp.remoteIP(); //Reset the foreflight IP address.
      Serial.print(m_foreflight_ip);
      // read the packet into packetBufffer. 
      // TODO: Check that it's actually FF and get the right port to transmit on.
      int len = udp.read(rx_buf, 255);
      Serial.println("Contents:");
    }
    vTaskDelay(ms_to_tick(5000)); //Wait 5 seconds, max frequency of inbound messages.
  }
}

static void TaskSendHeartbeatMsg( void *pvParameters ) {
  static unsigned char heartbeat_msg[] = {0x7E, 10, 0b10000001, 0, 0, 0, 0, 0, 0x7E};
  for(;;) {
    crc_inject(&heartbeat_msg[0], sizeof(heartbeat_msg));
    if (m_foreflight_ip != IPAddress(192,168,255,255)) { //TODO: make this a flag.
      udp.beginPacket(m_foreflight_ip, TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(heartbeat_msg); i++) {
        udp.write(heartbeat_msg[i]);
      }
      udp.endPacket();
    }
    vTaskDelay(ms_to_tick(1000));
  }
}
