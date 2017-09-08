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


const char MSG_PROMPT[] = "m> ";
const int MSG_PROMPT_LEN = 3;
const char MSG_VERSION[] = "1.0\r\n";
const int MSG_VERSION_LEN = 5;
const char MSG_UNKNOWN[] = "?\r\n";
const int MSG_UNKNOWN_LEN = 3;

const int XMODEM_RETRIES = 30;

// Start of Header
const uint8_t XMODEM_SOH = 0x01;

// End of Transmission
const uint8_t XMODEM_EOT = 0x04;

// Acknowledge
const uint8_t XMODEM_ACK = 0x06;

// Not Acknowledge (or start xmodem)
const uint8_t XMODEM_NACK = 0x15;

// ASCII ?C? (or start xmodem-crc)
const uint8_t XMODEM_C = 0x43;

// Cancel xmodem mode.
const uint8_t XMODEM_CTRLC = 0x03;

void WriteUsbSync(const uint8_t* data, uint8_t len) {
    putUSBUSART(SPI_CDC_PORT, (uint8_t*)data, len);
    do {
        CDCTxService();
    } while (!USBUSARTIsTxTrfReady(SPI_CDC_PORT));
}

void Delay1KTCYx(uint16_t x)
{
	do {
		_delay(1000);
	} while(--x != 0);
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

void SpiEnable(uint8_t enable)
{
    if (enable) {
        PROGB_TRIS = IO_DIRECTION_OUT;
        PROGB_LATCH = 0;

        Delay1KTCYx(1000);

		//Set SDO SPI Pin directions
		ANSELHbits.ANS9 = 0;// RC7 as digital IO
		ANSELHbits.ANS10 = 0;// RB4 as digital IO

		SI_TRIS = IO_DIRECTION_OUT;
		SO_TRIS = IO_DIRECTION_IN;
		CLK_TRIS = IO_DIRECTION_OUT;
        CS_TRIS = IO_DIRECTION_OUT;
        CS_LATCH = 1;

		//SSPSTAT settings
		SSPSTATbits.SMP = 0; //Sample at middle
		SSPSTATbits.CKE = 1;

        // Run at Fosc/4
        SSPADD = 0;

		//SSPCON1 Setings
		SSPCON1bits.CKP = 0;
		SSPCON1bits.SSPM3 = 0;
		SSPCON1bits.SSPM2 = 0;
		SSPCON1bits.SSPM1 = 1;
		SSPCON1bits.SSPM0 = 0;

		//Disable SSP interrupt
		PIE1bits.SSPIE = 0;

		//Enable MSSP
		SSPCON1bits.SSPEN = 1;

        Delay1KTCYx(1);
    } else {
        CloseSPI();

        CS_LATCH = 1;

		SI_TRIS = IO_DIRECTION_IN;
		SO_TRIS = IO_DIRECTION_IN;
		CLK_TRIS = IO_DIRECTION_IN;
        CS_TRIS = IO_DIRECTION_IN;

        Delay1KTCYx(1000);

        PROGB_LATCH = 1;
        PROGB_TRIS = IO_DIRECTION_IN;
    }
}

void ChipSelect(uint8_t sel) {
    if (sel) {
        CS_LATCH = 1;
        _delay(10);
        CS_LATCH = 0;
        _delay(10);
    } else {
        CS_LATCH = 1;
        _delay(10);
    }
}

uint8_t GetSpiFlashStatus() {
    ChipSelect(1);
    WriteSPI(0x05);
    uint8_t result = ReadSPI();
    ChipSelect(0);
    return result;
}

void WaitForWriteInProgress() {
    do {
        _delay(1000);
    } while(GetSpiFlashStatus() & 1);
}

void GetFlashId(void) {
    SpiEnable(1);

    ChipSelect(1);
    WriteSPI(0x9f);
    static uint8_t chipid[8];
    static int8_t i;
    chipid[6] = '\r';
    chipid[7] = '\n';
    getsSPI(chipid, 3);
    for (i = 2; i >= 0; --i) {
        chipid[i*2+1] = HexNybble(chipid[i] & 0xf);
        chipid[i*2] = HexNybble((chipid[i] >> 4) & 0xf);
    }
    ChipSelect(0);

    SpiEnable(0);

    WriteUsbSync(chipid, 8);
}

void BulkErase(void) {
    SpiEnable(1);

    // Write enable.
    ChipSelect(1);
    WriteSPI(0x06);
    ChipSelect(0);

    // Bulk erase.
    ChipSelect(1);
    WriteSPI(0xc7);
    ChipSelect(0);

    WaitForWriteInProgress();

    SpiEnable(0);
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
    Delay1KTCYx(100);
	PROGB_LATCH = 1;
    PROGB_TRIS = IO_DIRECTION_IN;
}

unsigned short XModemCrc16(uint8_t* data, uint8_t len) {
  const uint16_t poly = 0x1021;
  static uint16_t crc;
  static uint8_t i, j;

  crc = 0;
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

static uint8_t buf[134];
static const uint8_t* bufend = buf + sizeof(buf);

void Flash(void) {
    WriteUsbSync(&XMODEM_C, 1);

    static uint8_t r, len;
    static uint32_t write_addr;
    static uint16_t t, seq, crc;
    static uint8_t* next;

    r = 0;
    t = tick_count;
    next = buf;
    seq = 1;

    while (1) {
        len = getsUSBUSART(SPI_CDC_PORT, next, bufend - next);
        if (len == 0) {
            if (r == 0) {
                if (tick_count - t > 100) {
                  WriteUsbSync(&XMODEM_C, 1);
                  t = tick_count;
                }
            }
            continue;
        }

        if (r == 0) {
            SpiEnable(1);
            r = 1;
        }
        next += len;
        if (next > buf) {
            if (*buf == XMODEM_CTRLC) {
                break;
            } else if (*buf == XMODEM_SOH) {
                // Start of frame -- see if we have the full frame.
                if (next - buf >= (1 + 2 + 128 + 2)) {
                    next = buf;

                    crc = buf[1 + 2 + 128];
                    crc = crc << 8 | buf[1 + 2 + 128 + 1];
                    if (buf[1] == 255 - buf[2] && (seq & 0xff) == buf[1] && XModemCrc16(buf + 1 + 2, 128) == crc) {
                        if ((write_addr & 0x7ffff) == 0) {
                            ChipSelect(1);
                            WriteSPI(0x06);
                            ChipSelect(0);

                            // Sector erase.
                            ChipSelect(1);
                            WriteSPI(0xd8);
                            WriteSPI((write_addr >> 16) & 0xff);
                            WriteSPI((write_addr >> 8) & 0xff);
                            WriteSPI((write_addr >> 0) & 0xff);
                            ChipSelect(0);

                            WaitForWriteInProgress();
                        }

                        // Write enable.
                        ChipSelect(1);
                        WriteSPI(0x06);
                        ChipSelect(0);

                        ChipSelect(1);
                        WriteSPI(0x02);
                        WriteSPI((write_addr >> 16) & 0xff);
                        WriteSPI((write_addr >> 8) & 0xff);
                        WriteSPI((write_addr >> 0) & 0xff);
                        putbufSPI(buf + 1 + 2, 128);
                        ChipSelect(0);

                        WaitForWriteInProgress();

                        ChipSelect(1);
                        WriteSPI(0x03);
                        WriteSPI((write_addr>>16) & 0xff);
                        WriteSPI((write_addr>>8) & 0xff);
                        WriteSPI((write_addr>>0) & 0xff);
                        getsSPI(buf + 1 + 2, 128);
                        ChipSelect(0);

                        if (XModemCrc16(buf + 1 + 2, 128) == crc) {
                            seq += 1;
                            write_addr += 128;
                            WriteUsbSync(&XMODEM_ACK, 1);
                            continue;
                        }
                    }
                    WriteUsbSync(&XMODEM_NACK, 1);
                }
            } else if (*buf == XMODEM_EOT) {
                // End of file.
                WriteUsbSync(&XMODEM_ACK, 1);
                break;
            } else {
                // Frame started with something else.
                WriteUsbSync(&XMODEM_NACK, 1);
                next = buf;
                continue;
            }
        }
    }

    SpiEnable(0);
    Delay1KTCYx(10000);
}

void HandleCommand(uint8_t* cmd, uint8_t len) {
    switch (*cmd) {
        case '1':
            SpiEnable(1);
            break;
        case '0':
            SpiEnable(0);
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
        i = *(end+1);
        *(end) = '\r';
        *(end+1) = '\n';
        WriteUsbSync(next, end + 2 - next);
        *end = 0;
        *(end+1) = i;
        HandleCommand(buf, end - buf);
        next = buf;
        len = 0;

        sent_prompt = 0;
    } else {
        putUSBUSART(SPI_CDC_PORT, next, len);
        next += len;
    }
}
