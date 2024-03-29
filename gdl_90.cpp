#include "gdl_90.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "imu.h"
#include "defines.h"


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
static void TaskSendAHRS( void *pvParameters );
static void TaskSendID( void *pvParameters);
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
static void update_ahrs_msg(unsigned char* ahrs_msg_buf, size_t buflen, attitude_t attitude);
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
    ,  FAST_CORE);

 xTaskCreatePinnedToCore(
   TaskSendAHRS
   ,  "Send AHRS messages to Foreflight."
   ,  4096  // Stack size 
   ,  NULL
   ,  3  //Priority (0-3)
   ,  NULL 
   ,  FAST_CORE);

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "Listen for Foreflight JSON messages."
    ,  4096  // Stack size 
    ,  NULL
    ,  1  //Priority (0-3)
    ,  NULL 
    ,  FAST_CORE);
  xTaskCreatePinnedToCore(
    TaskSendID
    ,  "Send ID messages to Foreflight."
    ,  4096  // Stack size 
    ,  NULL
    ,  1  //Priority (0-3)
    ,  NULL 
    ,  FAST_CORE);
 xTaskCreatePinnedToCore(
   TaskSendAHRS
   ,  "Send AHRS report messages to Foreflight."
   ,  4096  // Stack size 
   ,  NULL
   ,  3  //Priority (0-3)
   ,  NULL 
   ,  FAST_CORE);
    
  
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

static void crc_inject(unsigned char *msg, size_t length) {
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
    vTaskDelay(MS_TO_TICK(5000)); //Wait 5 seconds, max frequency of inbound messages.
  }
}

static void TaskSendHeartbeatMsg( void *pvParameters ) {
  static unsigned char heartbeat_msg[] = {0x7E, 10, 0xFF, 0, 0, 0, 0, 0, 0x7E};
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
    vTaskDelay(MS_TO_TICK(1000));
  }
}

void TaskSendAHRS(void *pvParameters) {
  static uint8_t ahrs_msg[] = {0x7E, 0x65, 0x01, //Message header
    0x7F, 0xFF, //Roll
    0x7F, 0xFF, //Pitch
    0x7F, 0xFF, //Heading
    0x7F, 0xFF, //Knots Indicated Airspeed
    0xFF, 0xFF, //Knots True Airspeed
    0, 0, 0x7E //CRC and end bit.
  };

  for(;;) {
    if (m_foreflight_ip != IPAddress(192,168,255,255)) { //TODO: make this a flag.
      attitude_t attitude = get_attitude();
      update_ahrs_msg(&ahrs_msg[0], sizeof(ahrs_msg), attitude);
//      Serial.printf("msg[3] = %i, msg[4] = %i\n", ahrs_msg[3], ahrs_msg[4]);
      udp.beginPacket(m_foreflight_ip, TX_PORT);
      for(size_t i = 0; i < sizeof(ahrs_msg); i++) {
        udp.write(ahrs_msg[i]);
      }
      udp.endPacket();
      Serial.println("AHRS sent");
    }
    vTaskDelay(MS_TO_TICK(200));
  }
}

static void TaskSendID( void *pvParameters) {
  static uint8_t id_msg[] = {0x7E, 0x65, 0, 1, //Message header
    0, 0, 0, 0, 0, 0, 0, 0, //Serial number
    'A', 'H', 'R', 'S', 0x00, 0x00, 0x00, 0x00, //Device Name
    'B', 'a', 'c', 'k', 'u', 'p', ' ', 'A', //Device long name (16B)
    'H', 'R', 'S', 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x01, //Capabilities mask
    0x00, 0x00, 0x7E //Message tail
  };
  
  for(;;) {
    if (m_foreflight_ip != IPAddress(192,168,255,255)) { //TODO: make this a flag.
      crc_inject(&id_msg[0], sizeof(id_msg));
      udp.beginPacket(m_foreflight_ip, TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(id_msg); i++) {
        udp.write(id_msg[i]);
      }
      udp.endPacket();
    }
    vTaskDelay(MS_TO_TICK(200));
  }
}

static void update_ahrs_msg(unsigned char* ahrs_msg_buf, size_t buflen, attitude_t attitude) {
  //Foreflight website says big-endian, so I'm going to need to mask the raw ints. 
  ahrs_msg_buf[3] = (attitude.roll >> 8) & 0xFF;
  ahrs_msg_buf[4] = attitude.roll & 0xFF;
  ahrs_msg_buf[5] = (attitude.pitch >> 8) & 0xFF;
  ahrs_msg_buf[6] = attitude.pitch & 0xFF;
  ahrs_msg_buf[7] = (attitude.heading >> 8) & 0xFF;
  ahrs_msg_buf[8] = attitude.heading & 0xFF;
  crc_inject(ahrs_msg_buf, buflen);
  return;
}
