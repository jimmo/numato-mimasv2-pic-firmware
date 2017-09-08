#include <xc.h>

#include <stdint.h>

#include "usb_device_cdc.h"

#include "spi.h"
#include "spi_flash.h"

#include "system.h"

#define	SI_PORT			PORTCbits.RC7
#define	SO_PORT			PORTBbits.RB4
#define	CS_PORT			PORTCbits.RC2
#define	CLK_PORT		PORTBbits.RB6
#define	PROGB_PORT		PORTCbits.RC1
#define	CDONE_PORT		PORTCbits.RC0
#define	INITB_PORT		PORTCbits.RC5

#define	SI_LATCH		LATCbits.LATC7
#define	SO_LATCH		LATBbits.LATB4
#define	CS_LATCH		LATCbits.LATC2
#define	CLK_LATCH		LATBbits.LATB6
#define	PROGB_LATCH		LATCbits.LATC1
#define	CDONE_LATCH		LATCbits.LATC0
#define	INITB_LATCH		LATCbits.LATC5

#define	SI_TRIS			TRISCbits.TRISC7
#define	SO_TRIS			TRISBbits.TRISB4
#define	CS_TRIS			TRISCbits.TRISC2
#define	CLK_TRIS		TRISBbits.TRISB6
#define	PROGB_TRIS		TRISCbits.TRISC1
#define	CDONE_TRIS		TRISCbits.TRISC0
#define	INITB_TRIS		TRISCbits.TRISC5

#define IO_DIRECTION_OUT		0
#define IO_DIRECTION_IN			1


const char MSG_PROMPT[] = "mimas> ";
const int MSG_PROMPT_LEN = 7;
const char MSG_VERSION[] = "1.0\r\n";
const int MSG_VERSION_LEN = 5;
const char MSG_UNKNOWN[] = "?\r\n";
const int MSG_UNKNOWN_LEN = 3;

const int XMODEM_RETRIES = 30;

// Start of Header
const uint8_t XMODEM_SOH = 0x01;

// Start of Header (XMODEM-1k)
const uint8_t XMODEM_STX = 0x02;

// End of Transmission
const uint8_t XMODEM_EOT = 0x04;

// Acknowledge
const uint8_t XMODEM_ACK = 0x06;

// Not Acknowledge (or start xmodem)
const uint8_t XMODEM_NACK = 0x15;

// ASCII 'C' (or start xmodem-crc)
const uint8_t XMODEM_C = 0x43;

// Cancel xmodem mode.
const uint8_t XMODEM_CTRLC = 0x03;

void WriteUsbSync(const uint8_t* data, uint8_t len) {
    putUSBUSART(SPI_CDC_PORT, (uint8_t*) data, len);
    do {
        CDCTxService();
#if defined(USB_POLLING)
        USBDeviceTasks();
#endif
    } while (!USBUSARTIsTxTrfReady(SPI_CDC_PORT));
}

uint8_t HexNybble(uint8_t x) {
    if (x < 10) {
        return '0' + x;
    } else if (x < 16) {
        return 'a' + (x - 10);
    } else {
        return '?';
    }
}

void SpiEnable() {
    // Set FPGA to programming mode.
    PROGB_TRIS = IO_DIRECTION_OUT;
    PROGB_LATCH = 0;

    __delay_ms(1);

    //Set SDO SPI Pin directions
    ANSELHbits.ANS9 = 0; // RC7 as digital IO
    ANSELHbits.ANS10 = 0; // RB4 as digital IO

    // Set SPI pin modes.
    SI_TRIS = IO_DIRECTION_OUT;
    SO_TRIS = IO_DIRECTION_IN;
    CLK_TRIS = IO_DIRECTION_OUT;
    CS_TRIS = IO_DIRECTION_OUT;
    CS_LATCH = 1;

    // Configure MSSP
    SSPSTATbits.SMP = 0; // Sample at middle
    SSPSTATbits.CKE = 1; // Transmit active->idle.
    SSPCON1bits.CKP = 0;  // idle clock low.
    SSPCON1bits.SSPM = 1; // Fosc / 16

    //Disable SSP interrupt
    PIE1bits.SSPIE = 0;

    //Enable MSSP
    SSPCON1bits.SSPEN = 1;

    __delay_ms(1);
}

void SpiDisable() {
    // Turn of MSSP.
    SSPCON1bits.SSPEN = 0;

    CS_LATCH = 1;

    // Return SPI pins to hi-z.
    SI_TRIS = IO_DIRECTION_IN;
    SO_TRIS = IO_DIRECTION_IN;
    CLK_TRIS = IO_DIRECTION_IN;
    CS_TRIS = IO_DIRECTION_IN;

    __delay_ms(1);

    // Start FPGA.
    PROGB_LATCH = 1;
    PROGB_TRIS = IO_DIRECTION_IN;
}

void ChipSelect(uint8_t command) {
    CS_LATCH = 1;
    __delay_us(10);
    CS_LATCH = 0;
    __delay_us(10);
    SpiWriteByte(command);
}

void ChipDeselect() {
    CS_LATCH = 1;
    __delay_us(10);
}

uint8_t GetSpiFlashStatus() {
    ChipSelect(0x05);
    uint8_t result = SpiReadByte();
    ChipDeselect();
    return result;
}

void WaitForWriteInProgress() {
    // Deselects, then waits for the first bit of the status register to clear.
    CS_LATCH = 1;
    do {
        __delay_us(10);
    } while (GetSpiFlashStatus() & 1);
}

void GetFlashId(void) {
    SpiEnable();

    ChipSelect(0x9f);
    static uint8_t chipid[8];
    static int8_t i;
    chipid[6] = '\r';
    chipid[7] = '\n';
    SpiRead(chipid, 3);
    for (i = 2; i >= 0; --i) {
        chipid[i * 2 + 1] = HexNybble(chipid[i] & 0xf);
        chipid[i * 2] = HexNybble((chipid[i] >> 4) & 0xf);
    }
    ChipDeselect();

    SpiDisable();

    WriteUsbSync(chipid, 8);
}

void BulkErase(void) {
    SpiEnable();

    // Write enable.
    ChipSelect(0x06);
    ChipDeselect();

    // Bulk erase.
    ChipSelect(0xc7);
    WaitForWriteInProgress();

    SpiDisable();
}

void SpiFlashInit(void) {
    //Set all pins except PROGB as inputs
    SI_TRIS = IO_DIRECTION_IN;
    SO_TRIS = IO_DIRECTION_IN;
    CS_TRIS = IO_DIRECTION_IN;
    CLK_TRIS = IO_DIRECTION_IN;
    CDONE_TRIS = IO_DIRECTION_IN;
    INITB_TRIS = IO_DIRECTION_IN;

    //Pull PROG_B Low for a few milliseconds to start configuration
    PROGB_TRIS = IO_DIRECTION_OUT;
    PROGB_LATCH = 0;
    __delay_ms(5);
    PROGB_LATCH = 1;
    PROGB_TRIS = IO_DIRECTION_IN;
}

unsigned short XModemCrc16(uint8_t* data, uint8_t len, uint16_t crc) {
    const uint16_t poly = 0x1021;
    static uint8_t i, j;

    for (i = 0; i < len; ++i) {
        uint16_t x = data[i];
        crc = (crc ^ x << 8);
        for (j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = crc << 1 ^ poly;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

void SendAddress(uint8_t command, uint32_t addr) {
    ChipSelect(command);
    SpiWriteByte((addr >> 16) & 0xff);
    SpiWriteByte((addr >> 8) & 0xff);
    SpiWriteByte((addr >> 0) & 0xff);
}

static uint8_t buf[CDC1_DATA_IN_EP_SIZE];
static uint8_t verify_crc = 1;

void Flash(void) {
    WriteUsbSync(&XMODEM_C, 1);

    static uint8_t r, len, valid, op_size, avail;
    static uint32_t write_addr;
    static uint32_t page_addr;
    static uint16_t t, seq, frame_crc, data_crc, frame_size, remain, i, page_remaining;
    static uint8_t* start;

    r = 0;
    t = tick_count;
    seq = 1;
    write_addr = 0;

    while (1) {
        // Attempt to read first chunk of frame data.
        len = getsUSBUSART(SPI_CDC_PORT, buf, sizeof(buf));
        if (len == 0) {
            if (r == 0) {
                // Didn't get anything, and we haven't seen any frame yet, so
                // maybe send a 'C' to indicate that we're ready for xmodem-crc.
                if (tick_count - t > 100) {
                    // 100 ticks is roughly 1 second since last 'C' sent.
                    WriteUsbSync(&XMODEM_C, 1);
                    t = tick_count;
                }
            }
            continue;
        }

        if (r == 0) {
            // We have a (hopefully valid) first frame, switch to programming
            // mode.
            SpiEnable();
            r = 1;
        }

        if (*buf == XMODEM_CTRLC) {
            // Custom extension, allows aborting the flash mode at the prompt.
            break;
        } else if (*buf == XMODEM_STX || *buf == XMODEM_SOH && len > 5) {
            // Start of 1024 (STX) or 128 (SOH) byte frame.

            // If this is the start of a new sector (512kbit), then erase.
            if ((write_addr & 0xffff) == 0) {
                ChipSelect(0x06);
                ChipDeselect();

                // Sector erase.
                SendAddress(0xd8, write_addr);
                WaitForWriteInProgress();
            }

            valid = 0;
            frame_size = (*buf == XMODEM_STX) ? 1024 : 128;
            remain = frame_size;
            frame_crc = 0;
            data_crc = 0;
            page_addr = write_addr;

            // Verify that this is the expected sequence number. Only
            // write if it is valid, but continue to read all the data
            // for this frame regardless.
            if (buf[1] == 255 - buf[2] && (seq & 0xff) == buf[1]) {
                valid = 1;
            }
            // Offset the first read by the SOH/SEQ/~SEQ.
            start = buf+3;
            len -= 3;

            // Keep reading&writing untill we've done frame_size bytes.
            while (1) {
                if (len) {
                    // This will happen on the last read, where there will
                    // be the two CRC bytes extra on the end.
                    if (len > remain) {
                        len = remain;
                    }

                    // Only write to SPI flash if we're expecting this frame
                    // to be valid.
                    if (valid) {
                        // Update the CRC with this chunk.
                        data_crc = XModemCrc16(start, len, data_crc);

                        // Write the data.
                        // A write might straddle a page boundary, so it might
                        // need to be broken into multiple writes so this
                        // loop will either run once or twice.
                        avail = len;
                        while (avail > 0) {
                            // Write enable.
                            ChipSelect(0x06);
                            ChipDeselect();

                            // Write whatever's left of the current page.
                            page_remaining = (256 - (page_addr & 0xff));
                            op_size = avail;
                            if (op_size > page_remaining) {
                                // Write to the end of this page, leave
                                // remaining for next iteration.
                                op_size = page_remaining;
                            }

                            SendAddress(0x02, page_addr);
                            SpiWrite(start, op_size);
                            WaitForWriteInProgress();

                            // Advance to the start of the next page.
                            page_addr += op_size;
                            start += op_size;
                            avail -= op_size;

                            // assert((page_addr & 0xff) == 0)
                        }
                    }

                    // Update number of bytes left for this frame.
                    remain -= len;
                }

                // Frame complete. The next two bytes should be the CRC.
                if (remain == 0) {
                    frame_crc = buf[len];
                    frame_crc = frame_crc << 8 | buf[len + 1];
                    break;
                }

                // Get remaining frame bytes, but no more.
                op_size = sizeof(buf);
                if (remain + 2 < sizeof(buf)) {
                    op_size = remain + 2;
                }

                len = getsUSBUSART(SPI_CDC_PORT, buf, op_size);
                start = buf;
            }

            // Finished the frame, verify CRC against the received data.
            if (valid && frame_crc == data_crc) {
                if (verify_crc) {
                    // Then optionally verify against the written data.
                    data_crc = 0;

                    // Read back from SPI flash.
                    SendAddress(0x03, write_addr);
                    for (i = 0; i < frame_size; i += sizeof(buf)) {
                        SpiRead(buf, sizeof(buf));
                        data_crc = XModemCrc16(buf, sizeof(buf), data_crc);
                    }
                    ChipDeselect();
                } else {
                    // Verify disabled, skip by setting to the known-good CRC.
                    data_crc == frame_crc;
                }

                // Verify read-back CRC.
                if (frame_crc == data_crc) {
                    // Everything worked! Advance sequence number, the write
                    // address and ACK the frame.
                    seq += 1;
                    write_addr += frame_size;
                    WriteUsbSync(&XMODEM_ACK, 1);
                    continue;
                }
            }
        } else if (*buf == XMODEM_EOT) {
            // End of file, always ACK and go back to prompt.
            WriteUsbSync(&XMODEM_ACK, 1);
            break;
        }
        // Unknown frame, data was invalid, or verify failed.
        WriteUsbSync(&XMODEM_NACK, 1);
    }

    SpiDisable();
    __delay_ms(10);
}

void HandleCommand(uint8_t* cmd, uint8_t len) {
    switch (*cmd) {
        case '1':
            SpiEnable();
            break;
        case '0':
            SpiDisable();
            break;
        case 'f':
            Flash();
            break;
        case 'i':
            GetFlashId();
            break;
        case 'e':
            BulkErase();
            break;
        case 'c':
            verify_crc = 0;
            break;
        case 'C':
            verify_crc = 1;
            break;
        case 'v':
            WriteUsbSync(MSG_VERSION, MSG_VERSION_LEN);
            break;
        default:
            WriteUsbSync(MSG_UNKNOWN, MSG_UNKNOWN_LEN);
    }
}

void SpiFlashTask(void) {
    static int sent_prompt = 0;
    static uint8_t* next = buf;
    static uint8_t* end = buf;
    static uint8_t i;
    const static uint8_t* bufend = buf + sizeof(buf);

    if (!USBUSARTIsTxTrfReady(SPI_CDC_PORT)) {
        return;
    }

    if (!sent_prompt) {
        WriteUsbSync(MSG_PROMPT, MSG_PROMPT_LEN);
        sent_prompt = 1;
        return;
    }

    if (bufend == next) {
        next = buf;
        sent_prompt = 0;
        return;
    }

    static uint8_t len;
    len = getsUSBUSART(SPI_CDC_PORT, next, bufend - next);
    if (len == 0) {
        return;
    }

    end = NULL;
    // Screen/minicom send '\r', miniterm sends '\r\n'
    for (i = 0; i < len; ++i) {
        if (next[i] == '\r' || next[i] == '\n') {
            end = next + i;
            break;
        }
    }
    if (end) {
        i = *(end + 1);
        *(end) = '\r';
        *(end + 1) = '\n';
        WriteUsbSync(next, end + 2 - next);
        *end = 0;
        *(end + 1) = i;
        HandleCommand(buf, end - buf);
        next = buf;
        len = 0;

        sent_prompt = 0;
    } else {
        putUSBUSART(SPI_CDC_PORT, next, len);
        next += len;
    }
}
