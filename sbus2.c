#include "sbus2.h"



// ----- CIRCULAR BUFFER -----
static volatile uint8_t rx_buffer[64];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

static volatile uint8_t ch1val = 0;

void on_uart_rx(void);

// --------------------------
// 1) UART INIT + INTERRUPT SETUP
// --------------------------
void sbus_init(uint8_t tx_pin, uint8_t rx_pin) {
    // Configure pins for UART0
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    // Initialize the UART (documents: pico-sdk uart_init)
    uart_init(uart0, SBUS_BAUDRATE);

    // Set UART format: 8 data bits, 2 stop bits, even parity
    // (documents: pico-sdk uart_set_format)
    uart_set_format(uart0, 8, 2, UART_PARITY_EVEN);

    // Disable hardware flow control (CTS/RTS) (documents: pico-sdk uart_set_hw_flow)
    uart_set_hw_flow(uart0, false, false);

    // Enable the UART FIFOs (documents: pico-sdk uart_set_fifo_enabled)
    uart_set_fifo_enabled(uart0, true);

    // Set up RX interrupts (documents: pico-sdk uart_set_irq_enables, irq_set_exclusive_handler)
    uart_set_irq_enables(uart0, true, false);
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);

    printf("[INFO] SBUS UART initialized.\n");
}

// --------------------------
// 2) UART RX ISR
// --------------------------
void on_uart_rx() {
    // Read until hardware FIFO is empty (documents: pico-sdk uart_is_readable, uart_getc)
    while (uart_is_readable(uart0)) {
        uint8_t byte = uart_getc(uart0);
        uint8_t next_head = (rx_head + 1) % sizeof(rx_buffer);

        // Check for circular-buffer overflow
        if (next_head != rx_tail) {
            rx_buffer[rx_head] = byte;
            rx_head = next_head;
        } else {
            // Overflow condition
            printf("[ERROR] RX buffer overflow!\n");
        }
    }
}

// --------------------------
// 3) GET A COMPLETE SBUS FRAME FROM BUFFER
// --------------------------
bool get_sbus_frame(uint8_t *frame_out, size_t frame_len) {
    // Check if we have enough bytes in the circular buffer
    size_t available = (rx_head + sizeof(rx_buffer) - rx_tail) % sizeof(rx_buffer);
    if (available < frame_len) {
        return false; // Not enough data
    }

    // Copy frame_len bytes from the buffer
    for (size_t i = 0; i < frame_len; i++) {
        frame_out[i] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
    }
    return true;
}

// --------------------------
// 4) VALIDATE SBUS FRAME STRUCTURE
// --------------------------
bool validate_sbus_frame(const uint8_t *frame) {
    // Basic check: start + end bytes
    if (frame[0] != SBUS_START_BYTE) {
        printf("[ERROR] Invalid start byte: 0x%02X\n", frame[0]);
        return false;
    }
    if (frame[SBUS_FRAME_SIZE - 1] != SBUS_END_BYTE) {
        printf("[ERROR] Invalid end byte: 0x%02X\n", frame[SBUS_FRAME_SIZE - 1]);
        return false;
    }
    // Additional checks (failsafe bits, etc.) can be added if needed
    return true;
}

// --------------------------
// 5) DECODE SBUS CHANNEL DATA
// --------------------------
uint16_t sbus_get_channel(const uint8_t *frame, uint8_t channel_index) {
    // Each channel is 11 bits, starting at byte 1
    // Documented in many SBUS references, e.g. Futaba SBUS specs
    // No official "standard doc" from Futaba, but well-known community references

    // Calculate byte/bit offsets
    uint16_t byte_offset = 1 + (channel_index * 11) / 8;
    uint8_t  bit_offset  = (channel_index * 11) % 8;

    // Combine two bytes, then shift + mask
    uint16_t value = (frame[byte_offset] | (frame[byte_offset + 1] << 8)) >> bit_offset;
    value &= 0x07FF; // Only 11 bits
    return value;
}

// --------------------------
// 6) MAIN PROCESS LOOP
// --------------------------
void process_sbus_data() {
    while (true) {
        uint8_t frame[SBUS_FRAME_SIZE];

        if (get_sbus_frame(frame, SBUS_FRAME_SIZE)) {
            // We have a full 25-byte SBUS frame
            if (validate_sbus_frame(frame)) {
                printf("== SBUS FRAME RECEIVED ==\n");
                for (int ch = 0; ch < 16; ch++) {
                    uint16_t val = sbus_get_channel(frame, ch);
                    printf("Channel %d: %u\n", ch + 1, val);
                    ch1val = val;
                }
            } else {
                printf("[ERROR] Invalid SBUS frame\n");
            }
        }
        // If not enough data, just loop again
        // (Could add a sleep_ms(1) or so if needed)
    }
}


