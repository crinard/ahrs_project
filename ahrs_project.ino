#include <string.h>
#include "gdl_90.h"
#include "imu.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#define MS_TO_TICK(x) x/portTICK_PERIOD_MS //Arduino default ticks.
#define FAST_CORE 0
#define SLOW_CORE 1

void TaskBlink( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup(void) {
  Serial.begin(115200);
  gdl_90_init();
  imu_init();

  // blink task, for debugging.
  xTaskCreatePinnedToCore(
    TaskBlink
    ,  "TaskBlink"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL 
    ,  SLOW_CORE);
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
    vTaskDelay(MS_TO_TICK(100)); // On for 1/10 of s
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay(MS_TO_TICK(900));  // Wait 9/10 s.
  }
}

void loop() {} // Empty. Things are done in Tasks.
