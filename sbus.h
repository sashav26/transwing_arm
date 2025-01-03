#ifndef SBUS_H
#define SBUS_H

#include <stdbool.h>
#include <stdint.h>

// sbus protocol constants
#define SBUS_FRAME_SIZE     25
#define SBUS_START_BYTE     0x0F
#define SBUS_END_BYTE       0x00
#define SBUS_CHECKSUM_INDEX 23
#define SBUS_BAUDRATE       100000
#define SBUS_UART_TX_PIN    0   // adjust as needed
#define SBUS_UART_RX_PIN    1   // adjust as needed

// circular buffer size
#define RX_BUFFER_SIZE      128

// parser structure
typedef struct {
    uint8_t buffer[SBUS_FRAME_SIZE];
    uint16_t sbusChannels[18];  // 16 channels + 2 digital
    uint8_t validSbusFrame;
    uint8_t lostSbusFrame;
    uint8_t resyncEvent;
    bool isSync;
} Parser;

// function prototypes
void sbus_init(uint8_t tx_pin, uint8_t rx_pin);
float sbus_parse_channel1(Parser *p);
void sbus_print_buffer(const Parser *p);

#endif // SBUS_H
