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

/****** Tasks ******/

/**
   @brief Listens for ForeFlight IP address on port 63093.
*/
static void TaskGetFFIP (void *pvParameters );

/**
   @brief Sends Heartbeat message at 1Hz to all connected devices.
*/
static void TaskSendHeartbeatMsg( void *pvParameters );

/**
   @brief Sends Foreflight AHRS message at 5Hz to all connected devices.
*/
static void TaskSendAHRS( void *pvParameters );

/**
   @brief Sends Foreflight ID message at 1Hz to all connected devices.
*/
static void TaskSendID( void *pvParameters);

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

static void crc_inject(unsigned char *msg, size_t len) {
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
  for (;;) {
    for(size_t i = 0; i < MAX_CONNECTED_IP; i++) { //Process all of the incoming messages.
      int packetSize = udp.parsePacket();
      if (packetSize) {
        Serial.print("New transmission from:");
        Serial.print(udp.remoteIP());
        // read the packet into packetBufffer.
        int len = (packetSize < 255) ? udp.read(rx_buf, packetSize) : udp.read(rx_buf, 255);
        //TODO: Check for the correct JSON structure.
        Serial.println(rx_buf);
        m_ips[i] = udp.remoteIP(); //Add the ip to the list
        m_connected_nodes = i + 1; //Keep updating this, +1 to capture length, not indexing.
      } else {
        break;
      }
    }
    vTaskDelay(MS_TO_TICK(5000)); //Wait 5 seconds, max frequency of inbound messages.
  }
}

static void TaskSendHeartbeatMsg(void *pvParameters) {
  static unsigned char heartbeat_msg[] = {0x7E, 10, 0xFF, 0, 0, 0, 0, 0, 0x7E};
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
  static uint8_t ahrs_msg[] = {0x7E, 0x65, 0x01, //Message header
                               0x7F, 0xFF, //Roll
                               0x7F, 0xFF, //Pitch
                               0x7F, 0xFF, //Heading
                               0x7F, 0xFF, //Knots Indicated Airspeed
                               0xFF, 0xFF, //Knots True Airspeed
                               0, 0, 0x7E //CRC and end bit.
                              };

  for (;;) {
    ahrs_data_t attitude = get_attitude();
    update_ahrs_msg(&ahrs_msg[0], sizeof(ahrs_msg), attitude);
    for (size_t i = 0; i < m_connected_nodes; i++) {
      udp.beginPacket(m_ips[i], TX_PORT);
      #pragma unroll(full)
      for (size_t j = 0; j < sizeof(ahrs_msg); j++) {
        udp.write(ahrs_msg[j]);
      }
      udp.endPacket();
    }
    Serial.println("AHRS sent");
    vTaskDelay(MS_TO_TICK(200));
  }
}

static void TaskSendID(void *pvParameters) {
  static uint8_t id_msg[] = {0x7E, 0x65, 0, 1, //Message header
                             0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial number
                             0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device Name
                             0x61, 0x68, 0x72, 0x73, 0x00, 0x00, 0x00, 0x00, //Device long name (16B)
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                             0x00, 0x00, 0x00, 0x01, //Capabilities mask.
                             0x00, 0x00, 0x7E //CRC and Flag byte.
                            };
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
