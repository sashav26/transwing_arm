#include "sbus2.h"




static bool start = false;
static uint8_t frame[SBUS_FRAME_SIZE];
static uint8_t counter;


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
        printf("byte received: 0x%02X, start flag is %d\n", byte, start);
        if(start == false && byte != SBUS_START_BYTE){
            printf("line 52\n");
            continue;
        } //not saving bytes + not start byte, skip
        else if(start == false && byte == SBUS_START_BYTE){ //not saving bytes + start byte, flip flag
            printf("line 56, channels[0] = %d\n", channels[0]);
            counter = 0;
            frame[0] = byte;
            start = true;
        }
        else if (start == true){ //we have SEEN start byte, so everything until the END BYTE is to be recorded and parsed
            printf("line 62\n");
            frame[++counter] = byte;
            if( byte == SBUS_END_BYTE && counter == 24){
                printf("FLIPPING START!!! \n");
                counter = 0;
                start = false;
                channels[0] = sbus_get_channel(frame, 0);
            }

        }
        else{
            if(counter > 24){
                printf(" ERROR: COUNTER OVERFLOW");
            }
            printf("ERROR OCCURRED IN INTERRUPT SEQUENCE");
        }

    }
}



// --------------------------
// DECODE SBUS CHANNEL DATA
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



