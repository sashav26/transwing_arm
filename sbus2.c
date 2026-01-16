#include "sbus2.h"



// ----- CIRCULAR BUFFER -----
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;
static volatile uint8_t rx_tail = 0;

// ----- STATISTICS -----
static sbus_stats_t stats = {0};

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
            stats.buffer_overflows++;
            printf("[ERROR] RX buffer overflow! (Total: %u)\n", stats.buffer_overflows);
        }
    }
}

// --------------------------
// 3) CIRCULAR BUFFER HELPER FUNCTIONS
// --------------------------

// Get number of bytes available in circular buffer
static size_t buffer_available(void) {
    return (rx_head + sizeof(rx_buffer) - rx_tail) % sizeof(rx_buffer);
}

// Peek at a byte in the buffer without consuming it (offset from current tail position)
static uint8_t buffer_peek(size_t offset) {
    uint8_t pos = (rx_tail + offset) % sizeof(rx_buffer);
    return rx_buffer[pos];
}

// Consume (discard) one byte from the buffer
static void buffer_consume_one(void) {
    if (buffer_available() > 0) {
        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
    }
}

// Consume a full frame from the buffer
static void buffer_consume_frame(uint8_t *frame_out, size_t frame_len) {
    for (size_t i = 0; i < frame_len; i++) {
        frame_out[i] = rx_buffer[rx_tail];
        rx_tail = (rx_tail + 1) % sizeof(rx_buffer);
    }
}

// --------------------------
// 4) GET A COMPLETE SBUS FRAME WITH SYNCHRONIZATION
// --------------------------
bool get_sbus_frame(uint8_t *frame_out, size_t frame_len) {
    static bool was_synced = false;
    bool had_to_resync = false;

    // Keep searching for a valid frame until we find one or run out of data
    while (buffer_available() >= frame_len) {
        // Check if current position has start byte
        if (buffer_peek(0) != SBUS_START_BYTE) {
            buffer_consume_one(); // Discard and keep searching
            had_to_resync = true;
            continue;
        }

        // Found potential start byte, check if end byte is in correct position
        if (buffer_peek(frame_len - 1) != SBUS_END_BYTE) {
            buffer_consume_one(); // False start, keep searching
            had_to_resync = true;
            continue;
        }

        // Both start and end bytes look good - consume the full frame
        buffer_consume_frame(frame_out, frame_len);

        // Track synchronization events
        if (had_to_resync && was_synced) {
            stats.sync_events++;
            printf("[WARN] SBUS resync event #%u\n", stats.sync_events);
        }
        was_synced = true;

        return true;
    }

    // Not enough data available yet
    return false;
}

// --------------------------
// 5) VALIDATE SBUS FRAME STRUCTURE
// --------------------------
bool validate_sbus_frame(const uint8_t *frame) {
    // Note: Start and end bytes are already checked during synchronization
    // This function extracts additional status information from byte 23

    // Verify frame markers (should always pass if sync worked correctly)
    if (frame[0] != SBUS_START_BYTE || frame[SBUS_FRAME_SIZE - 1] != SBUS_END_BYTE) {
        printf("[ERROR] Frame validation failed (start: 0x%02X, end: 0x%02X)\n",
               frame[0], frame[SBUS_FRAME_SIZE - 1]);
        stats.frames_invalid++;
        return false;
    }

    // Extract status flags from byte 23
    uint8_t flags = frame[23];
    bool frame_lost = (flags & 0x04) != 0;    // Bit 2: Frame lost
    bool failsafe = (flags & 0x08) != 0;      // Bit 3: Failsafe active

    // Update statistics
    stats.failsafe_active = failsafe;
    stats.signal_lost = frame_lost;

    if (failsafe) {
        printf("[WARN] SBUS Failsafe active!\n");
    }
    if (frame_lost) {
        printf("[WARN] SBUS Frame lost flag set!\n");
    }

    return true;
}

// --------------------------
// 6) DECODE SBUS CHANNEL DATA
// --------------------------
uint16_t sbus_get_channel(const uint8_t *frame, uint8_t channel_index) {
    // Each channel is 11 bits, starting at byte 1
    // Documented in many SBUS references, e.g. Futaba SBUS specs
    // No official "standard doc" from Futaba, but well-known community references

    // Bounds check - SBUS has 16 analog channels (0-15)
    if (channel_index >= SBUS_NUM_CHANNELS) {
        printf("[ERROR] Invalid channel index: %u (max: %u)\n", channel_index, SBUS_NUM_CHANNELS - 1);
        return 0;
    }

    // Calculate byte/bit offsets
    uint16_t byte_offset = 1 + (channel_index * 11) / 8;
    uint8_t  bit_offset  = (channel_index * 11) % 8;

    // Combine two bytes, then shift + mask
    uint16_t value = (frame[byte_offset] | (frame[byte_offset + 1] << 8)) >> bit_offset;
    value &= SBUS_CHANNEL_MASK; // Only 11 bits
    return value;
}

// --------------------------
// 7) MAIN PROCESS LOOP
// --------------------------
void process_sbus_data() {
    while (true) {
        uint8_t frame[SBUS_FRAME_SIZE];

        if (get_sbus_frame(frame, SBUS_FRAME_SIZE)) {
            // We have a full 25-byte SBUS frame
            if (validate_sbus_frame(frame)) {
                stats.frames_received++;
                stats.last_valid_frame_us = time_us_64();

                printf("== SBUS FRAME #%u ==\n", stats.frames_received);
                for (int ch = 0; ch < SBUS_NUM_CHANNELS; ch++) {
                    uint16_t val = sbus_get_channel(frame, ch);
                    printf("Channel %d: %u\n", ch + 1, val);
                }
            } else {
                stats.frames_invalid++;
                printf("[ERROR] Invalid SBUS frame (total invalid: %u)\n", stats.frames_invalid);
            }
        } else {
            // Not enough data available, sleep briefly to prevent CPU spin
            sleep_us(100);
        }
    }
}

// --------------------------
// 8) NON-BLOCKING API FUNCTIONS
// --------------------------

// Check if a frame is available without consuming it
bool sbus_frame_available(void) {
    // Check if we have at least SBUS_FRAME_SIZE bytes
    if (buffer_available() < SBUS_FRAME_SIZE) {
        return false;
    }

    // Check if current position looks like a valid frame start/end
    if (buffer_peek(0) == SBUS_START_BYTE &&
        buffer_peek(SBUS_FRAME_SIZE - 1) == SBUS_END_BYTE) {
        return true;
    }

    return false;
}

// Read all channels into the provided array
bool sbus_read_channels(uint16_t channels[SBUS_NUM_CHANNELS]) {
    uint8_t frame[SBUS_FRAME_SIZE];

    // Try to get a frame
    if (!get_sbus_frame(frame, SBUS_FRAME_SIZE)) {
        return false; // No frame available
    }

    // Validate the frame
    if (!validate_sbus_frame(frame)) {
        stats.frames_invalid++;
        return false;
    }

    // Frame is valid - update statistics
    stats.frames_received++;
    stats.last_valid_frame_us = time_us_64();

    // Extract all channels
    for (int ch = 0; ch < SBUS_NUM_CHANNELS; ch++) {
        channels[ch] = sbus_get_channel(frame, ch);
    }

    return true;
}

// Read a single channel
bool sbus_read_single_channel(uint8_t channel_index, uint16_t *value) {
    uint16_t channels[SBUS_NUM_CHANNELS];

    if (!sbus_read_channels(channels)) {
        return false;
    }

    if (channel_index < SBUS_NUM_CHANNELS) {
        *value = channels[channel_index];
        return true;
    }

    return false;
}

// --------------------------
// 9) STATISTICS FUNCTIONS
// --------------------------
sbus_stats_t sbus_get_stats(void) {
    return stats;
}

void sbus_reset_stats(void) {
    stats.frames_received = 0;
    stats.frames_invalid = 0;
    stats.sync_events = 0;
    stats.buffer_overflows = 0;
    stats.last_valid_frame_us = 0;
    stats.failsafe_active = false;
    stats.signal_lost = false;
    printf("[INFO] SBUS statistics reset.\n");
}
