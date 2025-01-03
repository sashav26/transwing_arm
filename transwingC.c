// transwingC.c

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "servo.h"
#include "sbus.h"
#include <stdio.h>

// define servo pin (ensure this matches your setup)
#define SERVO_PIN 15

int main(){
    // initialize stdio for debugging
    stdio_init_all();

    // initialize servo on SERVO_PIN with start pulse width 1500us
    initServo(SERVO_PIN, 1500.0f);

    // initialize sbus on tx pin 2, rx pin 1 (adjust pins as needed)
    sbus_init(SBUS_UART_TX_PIN, SBUS_UART_RX_PIN);

    // initialize parser
    Parser parser = {};

    // wait for sbus to sync
    sleep_ms(1000);

    // main loop
    while (1) {
        // parse channel1 and get pulse width
        float pulse_width = sbus_parse_channel1(&parser);
        sbus_print_buffer(&parser);
        

        if (pulse_width > 0.0f) {
            // set servo pulse width based on channel1
            
            setPulseWidth(SERVO_PIN, pulse_width);
        } else {
            // handle error: no valid frame
            // optional: set failsafe pulse width or take other actions
            // printf("no valid sbus frame received.\n");
        }

        // optional: small delay to control loop rate
        sleep_ms(50); // adjust as needed (e.g., 20 ms for ~50 Hz)
    }

    return 0;
}
