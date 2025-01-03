#ifndef SBUS_H
#define SBUS_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

// ----- SBUS DEFINES -----
#define SBUS_BAUDRATE     100000     // Standard SBUS baud rate
#define SBUS_FRAME_SIZE   25         // Bytes per SBUS frame
#define SBUS_START_BYTE   0x0F       // Start-of-frame indicator
#define SBUS_END_BYTE     0x00       // End-of-frame indicator

void sbus_init(uint8_t tx_pin, uint8_t rx_pin);
void on_uart_rx(void);
bool get_sbus_frame(uint8_t *frame_out, size_t frame_len);
bool validate_sbus_frame(const uint8_t *frame);
uint16_t sbus_get_channel(const uint8_t *frame, uint8_t channel_index);
void process_sbus_data(void);
float channel1();


#endif