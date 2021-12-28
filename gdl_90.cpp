#include "gdl_90.h"
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include "imu.h"

#define MS_TO_TICK(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define FAST_CORE 0
#define SLOW_CORE 1
#define DEBUG 1
/****** Defines ******/
// Network stuff
#define NET_NAME "backup_ahrs"
#define NET_PASS "password"
#define RX_PORT 63093
#define TX_PORT 4000
#define MAX_CONNECTED_IP 5
#define CRC_DEFAULT 0x00
#define FLAG_BYTE 0x7E

// Message IDs
#define HEARTBEAT_MSG_ID 0
#define OWNSHIP_MSG_ID 10
#define TRAFFIC_MSG_ID 20
#define OWNSHIP_GEO_ALT_MSG_ID 11
#define UAT_UPLINK_MSG_ID 7
#define FF_AHRS_MSG_ID 0x65
#define FF_ID_MSG_ID 0x65

//Message lengths.
#define HEARTBEAT_MSG_LEN 11U
#define OWNSHIP_MSG_LEN 32U
#define TRAFFIC_MSG_LEN 32U
#define OWNSHIP_GEO_ALT_MSG_LEN 9U
#define UAT_UPLINK_MSG_LEN 440U
#define FF_AHRS_MSG_LEN 16U
#define FF_ID_MSG_LEN 43U

/****** Module Variables ******/
static uint16_t Crc16Table[256] = {0};
static WiFiUDP udp;
static IPAddress m_ips[MAX_CONNECTED_IP];
static size_t m_connected_nodes = 0;

/****** Function prototypes ******/

/**
   @brief Initializes crc table for Flag byte and CRC.
   @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
void crc_init(void);

/**
   @brief Adds Flag byte and CRC.
   @param msg The beginning of the message to be sent.
   @param len The length of the message buffer.
   @note See "GDL 90 Data Interface Specification, 560-1058-00, section 2.2" for details.
 **/
void crc_inject(unsigned char *msg, uint32_t len);

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
    ,  "1Hz msg"
    ,  4096 // Stack size
    ,  NULL
    ,  1 // Priority (0-3)
    ,  NULL
    ,  FAST_CORE);

  xTaskCreatePinnedToCore(
    TaskSendAHRS
    ,  "AHRS msg"
    ,  2048  // Stack size
    ,  NULL
    ,  2  //Priority (0-3)
    ,  NULL
    ,  FAST_CORE);

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "FF_RX"
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

void crc_inject(uint8_t *msg, size_t len) {
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
    m_connected_nodes = 0;
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
  static uint8_t heartbeat_msg[HEARTBEAT_MSG_LEN] = 
                                        {FLAG_BYTE, HEARTBEAT_MSG_ID,// Message header
                                         0xFF, 0x00, // Status bytes 1 & 2
                                         0x00, 0x00, // Timestamp
                                         0, 0, // Message counts
                                         0, 0, FLAG_BYTE //CRC and flag byte.
                                        };
  static uint8_t id_msg[FF_ID_MSG_LEN] = 
                                        {FLAG_BYTE, FF_ID_MSG_ID, 0, 1, //Message header
                                         0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, //Serial number
                                         'A', 'H', 'R', 'S', 0x00, 0x00, 0x00, 0x00, //Device Name
                                         'B', 'a', 'c', 'k', 'u', 'p', ' ', 'A', //Device long name (16B)
                                         'H', 'R', 'S', 0x00, 0x00, 0x00, 0x00, 0x00,
                                         0x00, 0x00, 0x00, 0x01, //Capabilities mask
                                         CRC_DEFAULT, CRC_DEFAULT, FLAG_BYTE //CRC and Flag byte
                                        };
  static uint8_t ownship_msg[OWNSHIP_MSG_LEN] = 
                                        {FLAG_BYTE, OWNSHIP_MSG_ID, 
                                         0x00, //Traffic alert and type of address
                                         0x00, 0x00, 0x00, //Participant address
                                         0x00, 0x00, 0x00, // Latitide (180/ 2^23 degrees resolution, signed)
                                         0x00, 0x00, 0x00, // Longitude (180/ 2^23 degrees resolution, signed)
                                         0x00, 0x00, // Altitude, in 25 ft resolution, and misc indicators.
                                         0x00, // NIC and NACp
                                         0x00, 0x00, 0x00, // Horizontal and Vertical velocity
                                         0x00, //Track | Heading.
                                         0x00, //Emitter catagory
                                         'm', 'y', 'c', 'a', //Callsign (8 bytes)
                                         'l', 's', 'g', 'n',
                                         0x00, //Emergency code and spare.
                                         CRC_DEFAULT, CRC_DEFAULT, FLAG_BYTE
                                         };

  // static uint8_t ownship_geo_alt_msg[OWNSHIP_GEO_ALT_MSG_LEN] = 
  //                                       {FLAG_BYTE, OWNSHIP_GEO_ALT_MSG_ID,
                                         
  //                                        CRC_DEFAULT, CRC_DEFAULT, FLAG_BYTE
  //                                       };
  crc_inject(id_msg, FF_ID_MSG_LEN);
  crc_inject(heartbeat_msg, HEARTBEAT_MSG_LEN);
  
  for (;;) {
    crc_inject(ownship_msg, OWNSHIP_MSG_LEN);
    // crc_inject(ownship_geo_alt_msg, OWNSHIP_GEO_ALT_MSG_LEN);
    for (size_t i = 0; i < m_connected_nodes; i++) {
      udp.beginPacket(m_ips[i], TX_PORT);
      udp.write(&id_msg[0], FF_ID_MSG_LEN);
      udp.write(&heartbeat_msg[0], HEARTBEAT_MSG_LEN);
      udp.write(&ownship_msg[0], OWNSHIP_MSG_LEN);
      udp.endPacket();
    }
    vTaskDelay(MS_TO_TICK(1000));
  }
}

void TaskSendAHRS(void *pvParameters) {
  static uint8_t ahrs_msg[FF_AHRS_MSG_LEN] = {FLAG_BYTE, FF_AHRS_MSG_ID, 0x01, //Message header
                                              0x7F, 0xFF, //Roll
                                              0x7F, 0xFF, //Pitch
                                              0x7F, 0xFF, //Heading
                                              0x7F, 0xFF, //Knots Indicated Airspeed
                                              0xFF, 0xFF, //Knots True Airspeed
                                              CRC_DEFAULT, CRC_DEFAULT, FLAG_BYTE //CRC and end bit.
                                              };
  for (;;) {
    for (size_t i = 0; i < m_connected_nodes; i++) {
      crc_inject(&ahrs_msg[0], sizeof(ahrs_msg));
      udp.beginPacket(m_ips[i], TX_PORT);
      for(size_t i = 0; i < sizeof(ahrs_msg); i++) {
         udp.write(ahrs_msg[i]);
      }
      udp.endPacket();
    }
    vTaskDelay(MS_TO_TICK(200));
  }
}
