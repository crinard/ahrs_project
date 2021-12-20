#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>
#include <WiFiUdp.h>
#include <string.h>

/* Builtins and generic defines */
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define ms_to_tick(x) x/portTICK_PERIOD_MS //Arduino default ticks.

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

/* WifiAP defines */
#define NET_NAME "hi there"
#define RX_PORT 63093
#define TX_PORT 4000
static WiFiUDP udp;

static uint16_t Crc16Table[256] = {0};
IPAddress g_foreflight_ip = IPAddress(192,168,255,255); //Default to very visible for debugging
#define GDL_90_OWNSHIP_MSG_LEN 32

void TaskBlink( void *pvParameters );
void TaskGetFFIP (void *pvParameters );
void TaskSendOwnshipMsg(void *pvParameters);
void TaskSendAHRS(void *pvParameters);

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

/**
 * @brief Handles setup common between wifi tasks, as well as AP setup function.
 **/
void setup_ip(void) {
  // WiFi AP setup, independent of tasks.
  WiFi.softAP(NET_NAME);
  udp.begin(RX_PORT);
  crc_init();
}
// the setup function runs once when you press reset or power the board
void setup(void) {
  
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  setup_ip();
  // blink task
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "TaskGetFFIP"   // A name just for humans
    ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);
    
  xTaskCreatePinnedToCore(
  TaskSendOwnshipMsg
  ,  "TaskSendOwnshipMsg"   // A name just for humans
  ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
  ,  NULL
  ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
  TaskSendAHRS
  ,  "TaskSendAHRS"   // A name just for humans
  ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
  ,  NULL
  ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL 
  ,  ARDUINO_RUNNING_CORE);
}

void loop()
{
  // Empty. Things are done in Tasks.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  pinMode(LED_BUILTIN, OUTPUT);
  for (;;) // A Task shall never return or exit. 
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay(ms_to_tick(100)); // On for 1/10 of s
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    Serial.println(g_foreflight_ip);
    vTaskDelay(ms_to_tick(900));  // Wait 9/10 s.
  }
}

void TaskGetFFIP (void *pvParameters ) {
  // Variable setup.
  static char rx_buf[255]; //buffer to hold incoming packet
  for (;;) {
    
    // if there's data available, read a packet
    int packetSize = udp.parsePacket();
    if (packetSize) {
      Serial.print("From ");
      g_foreflight_ip = udp.remoteIP(); //Reset the foreflight IP address.
      Serial.print(g_foreflight_ip);
      // read the packet into packetBufffer. 
      // TODO: Check that it's actually FF and get the right port to transmit on.
      int len = udp.read(rx_buf, 255);
      Serial.println("Contents:");
    }
    vTaskDelay(ms_to_tick(5000)); //Wait 5 seconds, max frequency of inbound messages.
  }
}

void TaskSendOwnshipMsg(void *pvParameters) {
  static unsigned char ownship_msg[] = {0x7E, 10, 0x01, 
    0x00, 0x00, 0x00,
    0x00, 0x20, 0x00, 
    0x40, 0x00, 0x00,
    0x00, 0xF2, 0x88,
    0x00, 0x10, 0x01,
    0x02,
    0x01,
    0x4E, 0x38, 0x32, 0x35, 0x56, 0x20, 0x20, 0x20, 
    0x00,
    0, 0, 0x7E
  }; // Initialize the ownship message to give GPS as 0/0.

  for(;;) {
    crc_inject(&ownship_msg[0], sizeof(ownship_msg));
    if (g_foreflight_ip != IPAddress(192,168,255,255)) { //TODO: make this a flag.
      udp.beginPacket(g_foreflight_ip, TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(ownship_msg); i++) {
        udp.write(ownship_msg[i]);
      }
      udp.endPacket();
      udp.beginPacket(IPAddress(192,168,4,255), TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(ownship_msg); i++) {
        udp.write(ownship_msg[i]);
      }
      udp.endPacket();
    }
    vTaskDelay(ms_to_tick(200));
  }
}

void TaskSendAHRS(void *pvParameters) {
  static unsigned char ahrs_msg[] = {0x7E, 0x65, 0x01, //Message header
    0x00, 0x00, //Roll
    0x00, 0x00, //Pitch
    0x00, 0x00, //Heading
    0x00, 0x00, //Knots Indicated Airspeed
    0x00, 0x00, //Knots True Airspeed
    0, 0, 0x7E //CRC and end bit.
  };

  for(;;) {
    crc_inject(ahrs_msg, sizeof(ahrs_msg));
    if (g_foreflight_ip != IPAddress(192,168,255,255)) { //TODO: make this a flag.
      udp.beginPacket(g_foreflight_ip, TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(ahrs_msg); i++) {
        udp.write(ahrs_msg[i]);
      }
      udp.endPacket();
      udp.beginPacket(IPAddress(192,168,4,255), TX_PORT);
      #pragma unroll(full)
      for(size_t i = 0; i < sizeof(ahrs_msg); i++) {
        udp.write(ahrs_msg[i]);
      }
      udp.endPacket();
    }
    vTaskDelay(ms_to_tick(200));
  }
}
