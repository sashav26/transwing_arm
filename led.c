// led.c
// LED control implementation

#include "led.h"
#include "config.h"
#include <stdio.h>

/**
 * @brief Initializes the onboard LED.
 */
void initLED(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // Turn off LED initially

    printf("[INFO] LED initialized on GPIO %d.\n", LED_PIN);
}

/**
 * @brief Toggles the onboard LED state.
 */
void blinkLED(void) {
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(LED_PIN, led_state);

    printf("[DEBUG] LED is now %s.\n", led_state ? "ON" : "OFF");
}

/**
 * @brief Sets the LED state explicitly.
 *
 * @param state true to turn LED on, false to turn LED off
 */
void setLED(bool state) {
    gpio_put(LED_PIN, state);
}
