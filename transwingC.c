// transwingC.c
// Main application for controlling servos via SBUS RC input

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "servo.h"
#include "sbus2.h"
#include "led.h"
#include "config.h"
#include <stdio.h>

/**
 * @brief Maps SBUS channel value (172-1811) to servo pulse width (1000-2000 us)
 *
 * @param sbus_value SBUS channel value (11-bit: 0-2047, typical range 172-1811)
 * @return float Servo pulse width in microseconds
 */
float map_sbus_to_servo(uint16_t sbus_value) {
    // Clamp to typical SBUS range
    if (sbus_value < SBUS_CHANNEL_MIN) sbus_value = SBUS_CHANNEL_MIN;
    if (sbus_value > SBUS_CHANNEL_MAX) sbus_value = SBUS_CHANNEL_MAX;

    // Map SBUS range (172-1811) to servo range (1000-2000 us)
    float normalized = (float)(sbus_value - SBUS_CHANNEL_MIN) /
                      (float)(SBUS_CHANNEL_MAX - SBUS_CHANNEL_MIN);

    return SERVO_MIN_US + normalized * (SERVO_MAX_US - SERVO_MIN_US);
}

int main() {
    // Initialize stdio for debugging
    stdio_init_all();

    printf("\n=== Transwing Arm Controller ===\n");
    printf("Initializing hardware...\n");

    // Initialize LED for status indication
    initLED();

    // Initialize servo on configured pin with center position
    initServo(SERVO_PIN, SERVO_CENTER_US);

    // Initialize SBUS on configured pins
    sbus_init(SBUS_TX_PIN, SBUS_RX_PIN);

    printf("Initialization complete. Waiting for SBUS data...\n\n");

    // Main control loop
    uint16_t channels[SBUS_NUM_CHANNELS];
    uint32_t last_stats_print = 0;

    while (true) {
        // Try to read SBUS channels
        if (sbus_read_channels(channels)) {
            // Successfully received frame - convert channel 1 to servo position
            float pulse_width = map_sbus_to_servo(channels[0]);
            setPulseWidth(SERVO_PIN, pulse_width);

            // Optional: Print channel values periodically (every 100 frames)
            sbus_stats_t stats = sbus_get_stats();
            if (stats.frames_received % 100 == 0) {
                printf("Frame #%u | Ch1: %u -> %.1f us | Failsafe: %s\n",
                       stats.frames_received,
                       channels[0],
                       pulse_width,
                       stats.failsafe_active ? "YES" : "NO");
            }
        }

        // Print statistics every 5 seconds
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_stats_print > 5000) {
            sbus_stats_t stats = sbus_get_stats();
            printf("\n--- SBUS Statistics ---\n");
            printf("Frames received: %u\n", stats.frames_received);
            printf("Invalid frames:  %u\n", stats.frames_invalid);
            printf("Sync events:     %u\n", stats.sync_events);
            printf("Buffer overflows: %u\n", stats.buffer_overflows);
            printf("Failsafe: %s | Signal lost: %s\n",
                   stats.failsafe_active ? "YES" : "NO",
                   stats.signal_lost ? "YES" : "NO");
            printf("----------------------\n\n");
            last_stats_print = now;
        }

        // Small delay to prevent tight looping
        sleep_ms(10);
    }

    return 0;
}
