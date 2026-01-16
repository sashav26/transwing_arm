// servo.h

#ifndef SERVO_H
#define SERVO_H

#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include <stdio.h>

// Initialize the servo on a specified GPIO pin with a starting pulse width in microseconds
void initServo(int servoPin, float startPulseWidth_us);

// Set the pulse width for the servo in microseconds
void setPulseWidth(int servoPin, float pulseWidth_us);

#endif // SERVO_H
