#ifndef SBUS_H
#define SBUS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// ----- SBUS DEFINES -----
#define SBUS_BAUDRATE      100000     // Standard SBUS baud rate
#define SBUS_FRAME_SIZE    25         // Bytes per SBUS frame
#define SBUS_START_BYTE    0x0F       // Start-of-frame indicator
#define SBUS_END_BYTE      0x00       // End-of-frame indicator
#define SBUS_NUM_CHANNELS  16         // Number of analog channels
#define SBUS_CHANNEL_MASK  0x07FF     // 11-bit channel value mask
#define RX_BUFFER_SIZE     64         // UART RX circular buffer size

// ----- SBUS STATISTICS STRUCTURE -----
typedef struct {
    uint32_t frames_received;       // Total valid frames received
    uint32_t frames_invalid;        // Frames that failed validation
    uint32_t sync_events;           // Number of times we had to resynchronize
    uint32_t buffer_overflows;      // Number of buffer overflow events
    uint64_t last_valid_frame_us;   // Timestamp of last valid frame (microseconds)
    bool failsafe_active;           // Current failsafe status
    bool signal_lost;               // Signal lost flag
} sbus_stats_t;

// ----- FUNCTION PROTOTYPES -----
void sbus_init(uint8_t tx_pin, uint8_t rx_pin);
void on_uart_rx(void);
bool get_sbus_frame(uint8_t *frame_out, size_t frame_len);
bool validate_sbus_frame(const uint8_t *frame);
uint16_t sbus_get_channel(const uint8_t *frame, uint8_t channel_index);

// Legacy blocking function - processes SBUS data in infinite loop
void process_sbus_data(void);

// ----- NON-BLOCKING API -----
// Check if a valid SBUS frame is available
bool sbus_frame_available(void);

// Read all 16 channels into the provided array (returns true if successful)
bool sbus_read_channels(uint16_t channels[SBUS_NUM_CHANNELS]);

// Read a single channel (returns true if successful)
bool sbus_read_single_channel(uint8_t channel_index, uint16_t *value);

// ----- STATISTICS API -----
// Get current SBUS statistics
sbus_stats_t sbus_get_stats(void);

// Reset SBUS statistics
void sbus_reset_stats(void);

#endif