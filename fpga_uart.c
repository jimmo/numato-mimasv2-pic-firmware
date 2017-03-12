#include <xc.h>

#include <stdint.h>

#include "fpga_uart.h"

#include "usb_device_cdc.h"

void FpgaUartInit(void) {
    // http://ww1.microchip.com/downloads/en/DeviceDoc/40001350F.pdf
    // Section 16.2

    // Tx on pin 10 -- RB7/TX/CK
    // Rx on pin 12 -- RB5/AN11/RX/DT

    // Disable analog on pin Rx pin.
    ANSELHbits.ANS11 = 0;

    // Set Tx as an output, and Rx as an input.
	TRISBbits.TRISB7 = 0;
	TRISBbits.TRISB5 = 1;

    // Asynchronous mode, 8-bit.
    TXSTA = 0;
    TXSTAbits.TXEN = 1;      // Transmit ENable.
    TXSTAbits.BRGH = 1;      // High Baud Rate select bit.

    RCSTA = 0;
    RCSTAbits.SPEN = 1;      // Serial Port ENable.
    RCSTAbits.CREN = 1;      // Continuous Reception ENable.

    BAUDCON = 0;
    BAUDCONbits.BRG16 = 1;   // 16-bit BRG enabled.

    // At 48MHz clock, with BRGH=BRG16=1.
    // 19200  : 624 (0x0270)
    // 115200 : 103 (0x0067)
    SPBRG = 0x70;
    SPBRGH = 0x02;

    // Read a byte.
    uint8_t c = RCREG;
}

// Important registers:
// RCSTAbits.OERR  --  Set if we didn't read RCREG before the next byte arrived.
// TXSTAbits.TRMT  --  Transmit shift register status bit.
// PIR1bits.RCIF   --  Receive interrupt flag, cleared when reading from RCREG.
// RCREG           --  Latest byte arrived on UART.
// TXREG           --  Send byte to UART.

// USB->UART
static uint8_t usb_rx_buf[CDC_DATA_IN_EP_SIZE] = {0};
static uint8_t usb_rx_pos = 0;
static uint8_t usb_rx_avail = 0;

// UART->USB
static uint8_t usb_tx_buf[CDC_DATA_OUT_EP_SIZE] = {0};
static uint8_t usb_tx_pos = 0;

void FpgaUartTask(void) {
    if (PIR1bits.RCIF) {
        // New byte available on UART.
        if (RCSTAbits.OERR) {
            // Receive overflow (e.g. because we weren't able to Tx over USB).
            // So throw away what we've received and reset the port.
            usb_tx_pos = 0;
            RCSTAbits.CREN = 0;
            usb_tx_buf[usb_tx_pos] = RCREG;
            RCSTAbits.CREN = 1;
        } else if (usb_tx_pos < sizeof(usb_tx_buf) - 1) {
            usb_tx_buf[usb_tx_pos] = RCREG;
        }
        ++usb_tx_pos;
    }

    if (usb_tx_pos > 0 && USBUSARTIsTxTrfReady(FPGA_UART_CDC_PORT)) {
        // We have data from the UART and the USB port is ready.
        putUSBUSART(FPGA_UART_CDC_PORT, usb_tx_buf, usb_tx_pos);
        usb_tx_pos = 0;
    }

    if (usb_rx_pos < usb_rx_avail) {
        // If there are still bytes from the last USB read.
        if (TXSTAbits.TRMT) {
            // And the port is ready to Tx, so send one byte.
            TXREG = usb_rx_buf[usb_rx_pos];
            ++usb_rx_pos;
        }
    } else {
        // Otherwise we grab more bytes from USB.
        usb_rx_avail = getsUSBUSART(FPGA_UART_CDC_PORT, usb_rx_buf, sizeof(usb_rx_buf));
        usb_rx_pos = 0;
    }
}
