#include <Arduino.h>
#include "drv_led.h"
#include "pins.h"

#define PIN_MODE_OUTPUT 0x01
#define ONE_SECOND 1000

error_t drv_led_init(void) {
    pinMode(BLUE_LED_PIN, PIN_MODE_OUTPUT);
    pinMode(RED_LED_PIN, PIN_MODE_OUTPUT);
    return ERROR_OK;
}

error_t blink_led(led_t led) {
    switch (led)
    {
        case (RED_LED):
            digitalWrite(RED_LED_PIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
            delay(ONE_SECOND/170); //TODO: make call non-blocking
            digitalWrite(RED_LED_PIN, HIGH);   // Turn the LED on (Note that LOW is the voltage level
            break;
        case (BLUE_LED):
            digitalWrite(BLUE_LED_PIN, LOW);
            delay(ONE_SECOND/170); //TODO: make call non-blocking
            digitalWrite(BLUE_LED_PIN, HIGH);
            break;
        default:
            return ERROR_BAD_INPUT;
    }
    return ERROR_OK;
}
