#ifndef DRV_LED_H
#define DRV_LED_H

#include "errors.h"

typedef enum led {
    RED_LED = 1,
    BLUE_LED = 2,
} led_t;

/**
 * @brief Initializes onboard LEDs
 */
error_t drv_led_init(void);

/**
 * @brief Blinks specified LED. Blinks for 1/100s.
 * @param led The LED to blink.
 */
error_t blink_led(led_t led);

#endif //DRV_LED_H