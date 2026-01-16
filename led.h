// led.h
// LED control functions for status indication

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "pico/stdlib.h"

// Initialize the onboard LED
void initLED(void);

// Toggle the onboard LED state
void blinkLED(void);

// Set LED state explicitly
void setLED(bool state);

#endif // LED_H
