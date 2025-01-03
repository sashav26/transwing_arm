// servo.c

#include "servo.h"


/* Internal Variables */
static float clockDiv = 64.0f;
static float wrap = 39062.0f;

/* LED Pin Definition */
#define LED_PIN 25  // Change if using a different GPIO for the onboard LED

/* SBUS Configuration */
#define SBUS_UART_ID uart0      // Using UART1
#define SBUS_BAUD_RATE 100000  // 100 kbps for SBUS
#define SBUS_DATA_BITS 8
#define SBUS_PARITY UART_PARITY_EVEN
#define SBUS_STOP_BITS 2

/* SBUS Frame Constants */
#define SBUS_START_BYTE 0x0F
#define SBUS_FRAME_SIZE 25

/* Function Prototypes */
void initLED();
void blinkLED();

/* SBUS Frame Buffer */
static uint8_t sbus_buffer[SBUS_FRAME_SIZE];
static int sbus_buffer_index = 0;

/**
 * @brief Initializes the PWM settings and the onboard LED.
 *
 * @param servoPin GPIO pin number connected to the servo.
 * @param startPulseWidth_us Initial pulse width in microseconds (e.g., 1500 for center position).
 */
void initServo(int servoPin, float startPulseWidth_us)
{
    // Initialize the Servo
    gpio_set_function(servoPin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(servoPin);

    pwm_config config = pwm_get_default_config();

    uint64_t clockspeed = clock_get_hz(clk_sys); // Use system clock
    clockDiv = 64.0f;
    wrap = clockspeed / clockDiv / 50.0f; // 50Hz for servo

    // Adjust clockDiv to ensure wrap fits within 16-bit limit
    while ((clockspeed / clockDiv / 50) > 65535.0f && clockDiv < 256.0f)
    {
        clockDiv += 64.0f;
    }
    wrap = clockspeed / clockDiv / 50.0f;

    // Debug: Print clockDiv and wrap values
    printf("Initializing Servo:\n");
    printf("Clock Speed: %llu Hz\n", clockspeed);
    printf("Clock Divider: %.1f\n", clockDiv);
    printf("Wrap Value: %.1f\n", wrap);

    pwm_config_set_clkdiv(&config, clockDiv);
    pwm_config_set_wrap(&config, (uint16_t)wrap);

    pwm_init(slice_num, &config, true);

    setPulseWidth(servoPin, startPulseWidth_us);

    // Initialize the LED
    initLED();
}

/**
 * @brief Sets the pulse width for the specified servo pin.
 *
 * @param servoPin GPIO pin number connected to the servo.
 * @param pulseWidth_us Pulse width in microseconds (typically between 1000 and 2000).
 */
void setPulseWidth(int servoPin, float pulseWidth_us)
{
    // Ensure pulseWidth_us is within typical servo range
    if (pulseWidth_us < 1000.0f)
        pulseWidth_us = 1000.0f;
    if (pulseWidth_us > 2000.0f)
        pulseWidth_us = 2000.0f;

    // Calculate the PWM level based on pulse width
    uint level = (pulseWidth_us / 20000.0f) * wrap;

    // Debug: Print the pulse width and PWM level
    printf("Setting pulse width to %.1f µs (PWM level: %u)\n", pulseWidth_us, level);

    pwm_set_gpio_level(servoPin, level);

    // Blink the LED to indicate servo movement
    blinkLED();
}

/**
 * @brief Initializes the onboard LED.
 */
void initLED()
{
    // Initialize the LED pin
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0); // Turn off LED initially

    // Debug: Print LED initialization status
    printf("LED initialized on GPIO %d.\n", LED_PIN);
}

/**
 * @brief Toggles the onboard LED state.
 *
 * This function turns the LED on if it's off, and vice versa.
 */
void blinkLED()
{
    static bool led_state = false;
    led_state = !led_state;
    gpio_put(LED_PIN, led_state);

    // Debug: Print LED state
    printf("LED is now %s.\n", led_state ? "ON" : "OFF");
}
