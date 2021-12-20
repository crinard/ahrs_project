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
#define RX_PORT 63093
#define TX_PORT 4000

const char *ssid = "yourAP";
static WiFiUDP udp;

char packetBuffer[255]; //buffer to hold incoming packet
char ReplyBuffer[] = "acknowledged";       // a string to send back

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskGetFFIP (void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  
  // blink task
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  2048  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  ARDUINO_RUNNING_CORE);

  // WiFi AP setup
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  udp.begin(63093);
  Serial.println("Server started");

  xTaskCreatePinnedToCore(
    TaskGetFFIP
    ,  "TaskGetFFIP"   // A name just for humans
    ,  4096  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
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

/*
  Blink
  Turns on an LED on for one second, then off for one second, repeatedly.
    
  If you want to know what pin the on-board LED is connected to on your ESP32 model, check
  the Technical Specs of your board.
*/

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) // A Task shall never return or exit. 
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay(ms_to_tick(100)); // On for 1/10 of s
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay(ms_to_tick(900));  // Wait 9/10 s.
  }
}

void TaskGetFFIP (void *pvParameters ) {
  for (;;) {
    // if there's data available, read a packet
    int packetSize = udp.parsePacket();
    if (packetSize) {
      Serial.print("Received packet of size ");
      Serial.println(packetSize);
      Serial.print("From ");
      IPAddress remoteIp = udp.remoteIP();
      Serial.print(remoteIp);
      Serial.print(", port ");
      Serial.println(udp.remotePort());
      // read the packet into packetBufffer
      int len = udp.read(packetBuffer, 255);
      if (len > 0) {
        packetBuffer[len] = 0;
      }
      Serial.println("Contents:");
      Serial.println(packetBuffer);
      // send a reply, to the IP address and port that sent us the packet we received
//      udp.beginPacket(udp.remoteIP(), udp.remotePort());
//      udp.write(ReplyBuffer);
//      udp.endPacket();
    }
  }
}
