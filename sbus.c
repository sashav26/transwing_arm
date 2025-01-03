#include "sbus.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"
#include <stdio.h>

// circular buffer variables
volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_head = 0;
volatile uint8_t rx_tail = 0;

// add byte to buffer
void rx_add_byte(uint8_t byte) {
    uint8_t next = (rx_head + 1) % RX_BUFFER_SIZE;
    if (next != rx_tail) {  // check for overflow
        rx_buffer[rx_head] = byte;
        rx_head = next;
    } else {
       // printf("-----ERROR-----: rx buffer overflow\n");
    }
}

// check if buffer not empty
bool rx_notempty() {
    return rx_head != rx_tail;
}

// get byte from buffer
uint8_t rx_get_byte() {
    if (!rx_notempty()) {
       // printf("-----ERROR-----: buffer empty\n");
        return 0;
    }
    uint8_t byte = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) % RX_BUFFER_SIZE;
    return byte;
}

// uart0 interrupt handler
void on_uart_rx() {
    while (uart_is_readable(uart0)) {
        uint8_t byte = uart_getc(uart0);
        rx_add_byte(byte);
    }
}

// initialize sbus uart
void sbus_init(uint8_t tx_pin, uint8_t rx_pin) {
    // set gpio functions
    gpio_set_function(tx_pin, GPIO_FUNC_UART);
    gpio_set_function(rx_pin, GPIO_FUNC_UART);

    // initialize uart0
    uart_init(uart0, SBUS_BAUDRATE);

    // set uart format: 8 data bits, 2 stop bits, even parity
    uart_set_format(uart0, 8, 2, UART_PARITY_EVEN);

    // disable hw flow control
    uart_set_hw_flow(uart0, false, false);

    // enable fifo
    uart_set_fifo_enabled(uart0, true);

    // set interrupt handler
    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);

    // enable uart rx interrupts
    uart_set_irqs_enabled(uart0, true, false);

    printf("-----INFO-----: sbus uart initialized successfully.\n");
}

// calculate sbus checksum
uint8_t sbus_checksum(const uint8_t *frame) {
    uint8_t checksum = 0;
    for(int i = 0; i < SBUS_CHECKSUM_INDEX; i++) {
        checksum += frame[i];
    }
    return checksum;
}

// parse first channel's pulse width
float sbus_parse_channel1(Parser *p) {
    static uint8_t frame_index = 0;
    static bool frame_started = false;

    while (rx_notempty()) {
        uint8_t byte = rx_get_byte();

        // debug: print received byte
        //printf("-----DEBUG-----: received byte: 0x%02X\n", byte);

        if (!frame_started) {
            if (byte == SBUS_START_BYTE) {
                // start of new frame
                frame_started = true;
                frame_index = 0;
                p->buffer[frame_index++] = byte;
                //printf("-----INFO-----: start byte detected.\n");
            }
        } else {
            p->buffer[frame_index++] = byte;

            if (frame_index == SBUS_FRAME_SIZE) {
                // check end byte
                if (p->buffer[SBUS_FRAME_SIZE - 1] == SBUS_END_BYTE) {
                    // calculate checksum
                    uint8_t calc_checksum = sbus_checksum(p->buffer);
                    uint8_t recv_checksum = p->buffer[SBUS_CHECKSUM_INDEX];

                    if (calc_checksum == recv_checksum) {
                        // valid frame
                        // extract channel 1 (11 bits from bytes 1 and 2)
                        uint16_t channel1 = ((p->buffer[1] | (p->buffer[2] << 8)) & 0x07FF);
                        printf("-----INFO-----: valid sbus frame received.\n");
                        printf("-----INFO-----: channel 1: %u\n", channel1);

                        // reset for next frame
                        frame_started = false;
                        frame_index = 0;

                        // convert to microseconds
                        return channel1 * 0.625f;
                    } else {
                        //printf("-----ERROR-----: invalid sbus frame (checksum mismatch).\n");
                    }
                } else {
                    // invalid end byte
                    //printf("-----ERROR-----: invalid sbus frame (incorrect end byte).\n");
                }

                // reset for next frame
                frame_started = false;
                frame_index = 0;
            }
        }
    }

    // no valid frame found
    return -1.0f;
}

// print sbus buffer
void sbus_print_buffer(const Parser *p) {
    //printf("-----DEBUG-----: sbus frame: ");
    for (int i = 0; i < SBUS_FRAME_SIZE; i++) {
        printf("0x%02X ", p->buffer[i]);
    }
    printf("\n");
}

// optional main function for testing
#ifdef SBUS_MAIN

int main() {
    // initialize stdio
    stdio_init_all();

    // wait for usb serial
    sleep_ms(1000);

    // initialize sbus
    sbus_init(SBUS_UART_TX_PIN, SBUS_UART_RX_PIN);

    // initialize parser
    Parser parser = {0};

    // main loop
    while (true) {
        float channel1 = sbus_parse_channel1(&parser);
        if (channel1 > 0) {
            printf("-----INFO-----: channel 1 pulse width: %.2f µs\n", channel1);
            sbus_print_buffer(&parser);
        }
        // small delay
        sleep_ms(10);
    }

    return 0;
}

#endif // SBUS_MAIN
