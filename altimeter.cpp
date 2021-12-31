#include <Adafruit_MPL3115A2.h>
#include "defines.h"
static Adafruit_MPL3115A2 baro;
static void task_read_alt(void *pvParams);
static float altitude;

void init_alt(void) {
    while (!baro.begin()) {
        Serial.println("Could not find sensor. Check wiring.");
    }
    baro.setSeaPressure(1013.26); //TODO: rewrite this, in inches of mercury.
    xTaskCreatePinnedToCore(
    task_read_alt
    ,  "Read Alt."
    ,  2048 // Stack size
    ,  NULL
    ,  1 // Priority (0-3)
    ,  NULL 
    ,  SLOW_CORE);
}

static void task_read_alt(void *pvParams) {
    for (;;) {
        altitude = baro.getAltitude();
        vTaskDelay(MS_TO_TICK(500)); //Read every half second.
        Serial.printf("Alt = %f\n", altitude);
    }
}

float get_baro_alt(void) {
    return altitude;
}
