#include <xc.h>

#include <stdint.h>
#include <stdio.h>

#include "usb_device_cdc.h"

#include "spi.h"
#include "spi_flash.h"

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



void Delay1KTCYx(unsigned char unit)
{
	do {
		_delay(1000);
	} while(--unit != 0);
}

void SpiEnable(uint8_t enable)
{
    if (enable) {
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
    }
}

void ChipSelect(uint8_t sel) {
    if (sel) {
            CS_LATCH = 1;
            _delay(1000);
            CS_LATCH = 0;
            _delay(1000);
    } else {
            CS_LATCH = 1;
            _delay(1000);
    }
}

uint8_t GetSpiFlashStatus() {
    ChipSelect(1);
    WriteSPI(0x05);
    uint8_t result = ReadSPI();
    ChipSelect(0);
    return result;
}

#define SPI_COMMAND_PROGRAMMING_MODE 1
#define SPI_COMMAND_GET_FLASH_ID 2
#define SPI_COMMAND_ERASE 3
#define SPI_COMMAND_PAGE_WRITE_START 4
#define SPI_COMMAND_PAGE_WRITE_END 5
#define SPI_COMMAND_PAGE_WRITE_DATA 6
#define SPI_COMMAND_WRITE 7
#define SPI_COMMAND_CHECKSUM_PAGE 8

typedef struct {
    uint8_t magic;
    uint8_t cmd;
    uint16_t size;
    uint8_t status;
    uint8_t data[32];
} SPI_COMMAND;

void ProcessCommand(SPI_COMMAND* cmd) {
    switch(cmd->cmd) {
        case SPI_COMMAND_PROGRAMMING_MODE:
            if (cmd->data[0]) {
                PROGB_TRIS = IO_DIRECTION_OUT;
                PROGB_LATCH = 0;
            } else {
                PROGB_LATCH = 1;
                PROGB_TRIS = IO_DIRECTION_IN;
            }
            cmd->status = 0;
            cmd->size = 0;
            break;

        case SPI_COMMAND_GET_FLASH_ID:
            SpiEnable(1);

            ChipSelect(1);
            WriteSPI(0x9f);
            getsSPI(cmd->data, 3);
            cmd->status = 0;
            cmd->size = 3;
            ChipSelect(0);

            SpiEnable(0);
            break;

        case SPI_COMMAND_ERASE:
            SpiEnable(1);

            uint32_t data_limit = 0;
            data_limit |= cmd->data[2];
            data_limit <<= 8;
            data_limit |= cmd->data[1];
            data_limit <<= 8;
            data_limit |= cmd->data[0];

            // Erase sector starting at address.
            for (uint32_t addr = 0; addr < data_limit; addr += 0x10000) {
                // Write enable.
                ChipSelect(1);
                WriteSPI(0x06);
                ChipSelect(0);

                ChipSelect(1);
                WriteSPI(0xd8);
                WriteSPI((addr>>16) & 0xff);
                WriteSPI((addr>>8) & 0xff);
                WriteSPI((addr>>0) & 0xff);
                ChipSelect(0);

                do {
                    _delay(100);
                } while (GetSpiFlashStatus() & 1);
            }

            // Write disable.
            ChipSelect(1);
            WriteSPI(0x04);
            ChipSelect(0);

            cmd->status = 0;
            cmd->size = 0;

            SpiEnable(0);
            break;

        case SPI_COMMAND_PAGE_WRITE_START:
            SpiEnable(1);

            // Write enable.
            ChipSelect(1);
            WriteSPI(0x06);
            ChipSelect(0);

            ChipSelect(1);
            WriteSPI(0x02);
            WriteSPI(cmd->data[2]);
            WriteSPI(cmd->data[1]);
            WriteSPI(cmd->data[0]);

            cmd->status = 0;
            cmd->size = 0;
            break;

        case SPI_COMMAND_PAGE_WRITE_END:
            ChipSelect(0);

            do {
                _delay(100);
            } while (GetSpiFlashStatus() & 1);

            // Write disable.
            ChipSelect(1);
            WriteSPI(0x04);
            ChipSelect(0);

            SpiEnable(0);

            cmd->status = 0;
            cmd->size = 0;
            break;

        case SPI_COMMAND_PAGE_WRITE_DATA:
            putbufSPI(cmd->data, cmd->size);
            cmd->status = 0;
            cmd->size = 0;
            break;

        case SPI_COMMAND_WRITE:
            SpiEnable(1);

            // Write enable.
            ChipSelect(1);
            WriteSPI(0x06);
            ChipSelect(0);

            ChipSelect(1);
            WriteSPI(0x02);
            WriteSPI(cmd->data[2]);
            WriteSPI(cmd->data[1]);
            WriteSPI(cmd->data[0]);

            putbufSPI(cmd->data+4, cmd->size-4);

            ChipSelect(0);

            do {
            } while (GetSpiFlashStatus() & 1);

            // Write disable.
            ChipSelect(1);
            WriteSPI(0x04);
            ChipSelect(0);

            SpiEnable(0);

            cmd->status = 0;
            cmd->size = 0;
            break;


        case SPI_COMMAND_CHECKSUM_PAGE:
            SpiEnable(1);

            ChipSelect(1);
            WriteSPI(0x03);
            WriteSPI(cmd->data[2]);
            WriteSPI(cmd->data[1]);
            WriteSPI(cmd->data[0]);

            uint32_t* sum = (uint32_t*)cmd->data;
            *sum = 0;
            for (uint16_t i = 0; i < 256; ++i) {
                *sum += ReadSPI();
            }

            ChipSelect(0);

            cmd->status = 0;
            cmd->size = 4;

            SpiEnable(0);
            break;

        default:
            break;
    }

    putUSBUSART(SPI_CDC_PORT, (uint8_t*)cmd, cmd->size+5);
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

// Data arriving from USB.
static uint8_t usb_rx_buf[sizeof(SPI_COMMAND)] = {0};
static uint16_t usb_rx_avail = 0;

void SpiFlashTask(void) {
    if (usb_rx_avail >= 5) {
        SPI_COMMAND* cmd = (SPI_COMMAND*)usb_rx_buf;
        if (cmd->magic == '~' && usb_rx_avail == cmd->size + 5 && USBUSARTIsTxTrfReady(SPI_CDC_PORT)) {
            ProcessCommand(cmd);
            usb_rx_avail = 0;
        }
        return;
    } else {
        uint8_t bytes_read = getsUSBUSART(SPI_CDC_PORT, usb_rx_buf + usb_rx_avail, sizeof(usb_rx_buf) - usb_rx_avail);
        if (bytes_read && usb_rx_buf[0] == '~') {
            usb_rx_avail += bytes_read;
        }
    }
}
