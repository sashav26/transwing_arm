// config.h
// Hardware configuration and constants for transwing_arm project

#ifndef CONFIG_H
#define CONFIG_H

// ----- HARDWARE PIN ASSIGNMENTS -----
#define SERVO_PIN       15      // GPIO pin for servo PWM output
#define LED_PIN         25      // Onboard LED (Raspberry Pi Pico)
#define SBUS_TX_PIN     0       // UART TX pin for SBUS (not typically used for RX-only)
#define SBUS_RX_PIN     1       // UART RX pin for SBUS input

// ----- SERVO CONFIGURATION -----
#define SERVO_MIN_US    1000.0f // Minimum servo pulse width (microseconds)
#define SERVO_MAX_US    2000.0f // Maximum servo pulse width (microseconds)
#define SERVO_CENTER_US 1500.0f // Center position pulse width (microseconds)
#define SERVO_FREQ_HZ   50.0f   // Servo PWM frequency (50Hz standard)

// ----- SBUS CONFIGURATION -----
#define SBUS_TIMEOUT_MS       1000   // Signal loss timeout (milliseconds)
#define SBUS_CHANNEL_MIN      172    // Minimum SBUS channel value (11-bit: 0-2047, typical range 172-1811)
#define SBUS_CHANNEL_MAX      1811   // Maximum SBUS channel value
#define SBUS_CHANNEL_CENTER   992    // Center SBUS channel value

// ----- DEBUG CONFIGURATION -----
// Uncomment to enable verbose debug output
// #define DEBUG_SBUS
// #define DEBUG_SERVO

#ifdef DEBUG_SBUS
    #define SBUS_DEBUG(...) printf(__VA_ARGS__)
#else
    #define SBUS_DEBUG(...)
#endif

#ifdef DEBUG_SERVO
    #define SERVO_DEBUG(...) printf(__VA_ARGS__)
#else
    #define SERVO_DEBUG(...)
#endif

#endif // CONFIG_H
