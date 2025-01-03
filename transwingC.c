// transwingC.c

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "servo.h"
#include "sbus2.h"
#include <stdio.h>

// define servo pin (ensure this matches your setup)
#define SERVO_PIN 15

int main(){
    // initialize stdio for debugging
    stdio_init_all();

    // initialize servo on SERVO_PIN with start pulse width 1500us
    initServo(SERVO_PIN, 1500.0f);

    // initialize sbus on tx pin 2, rx pin 1 (adjust pins as needed)
    sbus_init(0, 1);


    process_sbus_data();
    return 0;
}
