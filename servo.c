// servo.c

#include "servo.h"
#include "led.h"
#include "config.h"

/* Internal Variables */
static float clockDiv = 64.0f;
static float wrap = 39062.0f;

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
    wrap = clockspeed / clockDiv / SERVO_FREQ_HZ;

    // Adjust clockDiv to ensure wrap fits within 16-bit limit
    while ((clockspeed / clockDiv / SERVO_FREQ_HZ) > 65535.0f && clockDiv < 256.0f)
    {
        clockDiv += 64.0f;
    }
    wrap = clockspeed / clockDiv / SERVO_FREQ_HZ;

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
    if (pulseWidth_us < SERVO_MIN_US)
        pulseWidth_us = SERVO_MIN_US;
    if (pulseWidth_us > SERVO_MAX_US)
        pulseWidth_us = SERVO_MAX_US;

    // Calculate the PWM level based on pulse width
    uint level = (pulseWidth_us / 20000.0f) * wrap;

    // Debug: Print the pulse width and PWM level
    printf("Setting pulse width to %.1f µs (PWM level: %u)\n", pulseWidth_us, level);

    pwm_set_gpio_level(servoPin, level);

    // Blink the LED to indicate servo movement
    blinkLED();
}
