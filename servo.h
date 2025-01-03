// servo.h

#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "hardware/uart.h"
#include <stdio.h>

// Initialize the servo on a specified GPIO pin with a starting pulse width in microseconds
void initServo(int servoPin, float startPulseWidth_us);

// Set the pulse width for the servo in microseconds
void setPulseWidth(int servoPin, float pulseWidth_us);

// Initialize the onboard LED
void initLED(void);

// Toggle the onboard LED
void blinkLED(void);

// Initialize SBUS communication on UART1 RX (GPIO5)
void initSBUS(uint32_t uart_number, uint32_t tx_pin, uint32_t rx_pin);

// Read the first SBUS channel and return the corresponding pulse width in microseconds
float readSBUS_channel1_pulsewidth(void);

#endif // SERVO_H
