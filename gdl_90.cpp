#include "gdl_90.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "imu.h"

#define MS_TO_TICK(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define FAST_CORE 0
#define SLOW_CORE 1

/****** Defines ******/
#define NET_NAME "backup_ahrs"
#define NET_PASS "password"
#define RX_PORT 63093
#define TX_PORT 4000
#define MAX_CONNECTED_IP 5

/****** Module Variables ******/
static uint16_t Crc16Table[256] = {0};
static WiFiUDP udp;
static IPAddress m_ips[MAX_CONNECTED_IP];
static size_t m_connected_nodes = 0;

/****** Function Prototypes ******/
void crc_init(void);
void crc_inject(unsigned char *msg, size_t len);
/****** Tasks ******/

/**
   @brief Listens for ForeFlight IP address on port 63093.
*/
static void TaskGetFFIP (void *pvParameters );

/**
   @brief Sends Heartbeat, ID, and Ownship messages at 1Hz to all connected devices.
*/
static void TaskSend1hzMsgs(void *pvParameters);

/**
   @brief Sends Foreflight AHRS message at 5Hz to all connected devices.
*/
static void TaskSendAHRS(void *pvParameters);

/****** Function prototypes ******/

/**
   @brief Initializes crc table for Flag byte and CRC.
   @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
static void crc_init(void);

/**
   @brief Adds Flag byte and CRC.
   @param msg The beginning of the message to be sent.
   @param len The length of the message buffer.
   @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
static void crc_inject(unsigned char *msg, uint32_t len);
/**
   @brief Prepares the ahrs message for sending, adds attitude information, Flag byte, and CRC.
   @param ahrs_msg_buf Pointer to the ahrs message
   @param len The length of the message buffer.
   @param attitude The latest attitude measurement.
 **/
static void update_ahrs_msg(unsigned char* ahrs_msg_buf, size_t len, ahrs_data_t attitude);

/**
   @brief Initializes all components needed for GDL90 w/ foreflight.
 **/
void gdl_90_init(void) {
  // WiFi AP setup, independent of tasks.
  WiFi.softAP(NET_NAME);
  udp.begin(RX_PORT);
  crc_init();

  xTaskCreatePinnedToCore(
    TaskSend1hzMsgs
    ,  "Send 1Hz messages to Foreflight."
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
    ,  2  //Priority (0-3)
    ,  NULL
    ,  FAST_CORE);

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "Listen for Foreflight JSON messages."
    ,  4096  // Stack size
    ,  NULL
    ,  0  //Priority (0-3)
    ,  NULL
    ,  FAST_CORE);
}

void crc_init(void) {
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

void crc_inject(unsigned char *msg, size_t len) {
  uint32_t i;
  uint16_t crc = 0;
  // Return – CRC of the block
  // i – Starting address of message
  // i – Length of message
  for (i = 1; i < len - 3; i++) {
    crc = Crc16Table[crc >> 8] ^ (crc << 8) ^ msg[i];
  }
  //Now we have the crc, put it in the bytes third from last and second from last
  msg[len - 3] = (uint8_t)(crc & 0x00FF);
  msg[len - 2] = (uint8_t)((crc & 0xFF00) >> 8);
}

static void TaskGetFFIP(void *pvParameters) {
  // Variable setup.
  static char rx_buf[255]; //buffer to hold incoming packet
  static const char expected_msg[] = "{\"App\":\"ForeFlight\",\"GDL90\":{\"port\":4000}}";
  for (;;) {
    for(size_t i = 0; i < MAX_CONNECTED_IP; i++) { //Process all of the incoming messages.
      int packetSize = udp.parsePacket();
      if (packetSize) {
        udp.read(rx_buf, sizeof(expected_msg));
        bool matches = true;
        #pragma unroll(full)
        for (size_t j = 0; j < sizeof(expected_msg); j++) {
          matches = !((rx_buf[j] != expected_msg[j]) || !matches) ? true : false;
        }
        #ifdef DEBUG
          Serial.print("New transmission from:");
          Serial.println(udp.remoteIP());
          Serial.println(rx_buf);
          Serial.printf("Buffers match %i\n", matches);
        #endif
        m_ips[i] = udp.remoteIP(); //Add the ip to the list
        m_connected_nodes = i + 1; //Keep updating this, +1 to capture length, not indexing.
      } else {
        break;
      }
    }
    vTaskDelay(MS_TO_TICK(5000)); //Wait 5 seconds, max frequency of inbound messages.
  }
}

static void TaskSend1hzMsgs(void *pvParameters) {
  static uint8_t heartbeat_msg[] = {0x7E, 10, 0xFF, 0, 0, 0, 0, 0, 0x7E};
  static uint8_t id_msg[] =        {0x7E, 0x65, 0, 1, //Message header
                                    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial number
                                    0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device Name
                                    0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device long name (16B)
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0x01, //Capabilities mask
                                    0x00, 0x00, 0x7E //CRC and Flag byte
                                   };
  static uint8_t ownship_msg[] =   {0x7E, 10, //Message header
                                    0x01, // Traffic Alert Status and Address Type
                                    0x00, 0x00, 0x00, // Participant Address
                                    0x00, 0x20, 0x00, // Latitude: 24-bit signed binary fraction. Resolution = 180 / 223 degrees
                                    0x40, 0x00, 0x00, // Longitude: 24-bit signed binary fraction. Resolution = 180 / 223 degrees.
                                    0x00, 0xF2, 0x88, // Altitude: 12-bit offset integer. Resolution = 25 feet. Altitude (ft) = ("ddd" * 25) - 1,000, and Miscellaneous indicators: Navigation Integrity Category
                                    0x00, // Navigation Integrity Category (NIC), Navigation Accuracy Category for Position (NACp)
                                    0x10, 0x01, 0x02, //Horizontal velocity. Resolution = 1 kt, Vertical Velocity: Signed integer in units of 64 fpm.
                                    0x01, // Track/Heading: 8-bit angular weighted binary. Resolution = 360/256 degrees. 0 = North, 128 = South. See Miscellaneous field for Track/Heading indication.
                                    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, // Callsign
                                    0x00, // Emergency/Priority code, space.
                                    0, 0, 0x7E //CRC and Flag byte.
                                   };
  for (;;) {
    crc_inject(&heartbeat_msg[0], sizeof(heartbeat_msg));
    for (size_t i = 0; i < m_connected_nodes; i++) {
      udp.beginPacket(m_ips[i], TX_PORT);
      #pragma unroll(full)
      for (size_t j = 0; j < sizeof(heartbeat_msg); j++) {
        udp.write(heartbeat_msg[j]);
      }
      udp.endPacket();
    }
    vTaskDelay(MS_TO_TICK(1000));
  }
}

void TaskSendAHRS(void *pvParameters) {
  ff_ahrs_msg msg;
  for (;;) {
    ahrs_data_t attitude = get_attitude();
    msg.send();    
    vTaskDelay(MS_TO_TICK(200));
  }
}

static void TaskSendID(void *pvParameters) {
  
  crc_inject(&id_msg[0], sizeof(id_msg));
  // Only need to inject once, message will be constant.
  for (;;) {
    for (size_t i = 0; i < m_connected_nodes; i++) {
      udp.beginPacket(m_ips[i], TX_PORT);
      #pragma unroll(full)
      for (size_t j = 0; j < sizeof(id_msg); j++) {
        udp.write(id_msg[j]);
      }
      udp.endPacket();
    }
    vTaskDelay(MS_TO_TICK(200));
  }
}

static void update_ahrs_msg(unsigned char* ahrs_msg_buf, size_t buflen, ahrs_data_t attitude) {
  ahrs_msg_buf[3] = (attitude.roll >> 8) & 0xFF;
  ahrs_msg_buf[4] = attitude.roll & 0xFF;
  ahrs_msg_buf[5] = (attitude.pitch >> 8) & 0xFF;
  ahrs_msg_buf[6] = attitude.pitch & 0xFF;
  ahrs_msg_buf[7] = (attitude.heading >> 8) & 0xFF;
  ahrs_msg_buf[8] = attitude.heading & 0xFF;
  crc_inject(ahrs_msg_buf, buflen);
  return;
}
