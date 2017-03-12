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

        Delay1KTCYx(100);
    } else {
        CloseSPI();

        CS_LATCH = 1;

		SI_TRIS = IO_DIRECTION_IN;
		SO_TRIS = IO_DIRECTION_IN;
		CLK_TRIS = IO_DIRECTION_IN;
        CS_TRIS = IO_DIRECTION_IN;
    }
}
/*

void ServicePacket(PTR_SPARTAN_3A_CONFIG_OUT_PACKET ptrPacket)
{
    static char buf[14];
	//memset(&Packet, 0, sizeof(SPARTAN_3A_CONFIG_IN_PACKET));
	if(ptrPacket->RawPacket.Byte0 != '~')
	{
		//Incorrect packet delimiter
        return;
	}

    if (ptrPacket->SpiOpen.SpiNum != 1) {
        return;
    }

	if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_OPEN)
	{
        SPIOpen(ptrPacket->SpiOpen.SyncMode, ptrPacket->SpiOpen.BusMode, ptrPacket->SpiOpen.SmpPhase);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SpiOpen.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_CLOSE)
	{
        CloseSPI();
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SPIClose.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_GETSTRING)
	{
        PtrRespPacket->SPIBuffer.Tilda = '~';
		PtrRespPacket->SPIBuffer.PacketType = SPARTAN_3A_CONFIG_IN_PACKET_BUFFER;
		PtrRespPacket->SPIBuffer.SpiNum = ptrPacket->SPIGetString.SpiNum;
		getsSPI(&PtrRespPacket->SPIBuffer.Data[0], ptrPacket->SPIGetString.Length);
		USBSendPacket((unsigned char*)&PtrRespPacket->RawData[0], sizeof(SPARTAN_3A_CONFIG_IN_PACKET));
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_PUTSTRING)
	{
        putbufSPI(&ptrPacket->SPIPutString.Data[0], ptrPacket->SPIPutString.Length);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SPIPutString.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_GET_CHAR)
	{
        PtrRespPacket->SPIBuffer.Tilda = '~';
		PtrRespPacket->SPIBuffer.PacketType = SPARTAN_3A_CONFIG_IN_PACKET_BUFFER;
		PtrRespPacket->SPIBuffer.SpiNum = ptrPacket->SPIGetChar.SpiNum;
		PtrRespPacket->SPIBuffer.Data[0] = ReadSPI();
		USBSendPacket((unsigned char*)&PtrRespPacket->RawData[0], sizeof(SPARTAN_3A_CONFIG_IN_PACKET));
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_PUT_CHAR)
	{
        WriteSPI(ptrPacket->SPIPutChar.Data);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SPIPutChar.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_SET_IO_DIR)
	{
        SPISetIODirection(ptrPacket->SetIODir.Io, ptrPacket->SetIODir.Direction);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SetIODir.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_SET_IO_VALUE)
	{
        SPISetIOValue(ptrPacket->SetIOValue.Io, ptrPacket->SetIOValue.Value);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SetIOValue.SpiNum, ptrPacket->RawPacket.Byte1);
	}
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_GET_IO_VALUE)
	{
        PtrRespPacket->SPIBuffer.Tilda = '~';
		PtrRespPacket->SPIBuffer.PacketType = SPARTAN_3A_CONFIG_IN_PACKET_BUFFER;
		PtrRespPacket->SPIBuffer.SpiNum = ptrPacket->GetIOValue.SpiNum;
		PtrRespPacket->SPIBuffer.Data[0] = SPIGetIOValue(ptrPacket->GetIOValue.Io);
		USBSendPacket((unsigned char*)&PtrRespPacket->RawData[0], sizeof(SPARTAN_3A_CONFIG_IN_PACKET));
	}
}

void USBSendStatus(SPARTAN_3A_CONFIG_STATUS Status, uint8_t SPINum, uint8_t ExecutedCmd)
{
	PtrRespPacket->Status.Tilda = '~';
	PtrRespPacket->Status.PacketType = SPARTAN_3A_CONFIG_IN_PACKET_STATUS;
	PtrRespPacket->Status.SpiNum = SPINum;
	PtrRespPacket->Status.Status = (unsigned char)Status;
	PtrRespPacket->Status.ExecutedCmd = ExecutedCmd;

	USBSendPacket((unsigned char*)(char*)&PtrRespPacket->RawData[0], sizeof(SPARTAN_3A_CONFIG_IN_PACKET));
}

unsigned char USBSendPacket(unsigned char* data, unsigned char Size)
{
	putUSBUSART(SPI_CDC_PORT, (char*)data, Size);
    return 0;
}

*/

#define SPI_COMMAND_PROGRAMMING_MODE 1
#define SPI_COMMAND_GET_FLASH_ID 2

typedef struct {
    uint8_t magic;
    uint8_t cmd;
    uint16_t size;
    uint8_t status;
    uint8_t data[128];
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

            CS_LATCH = 1;
        Delay1KTCYx(100);
            CS_LATCH = 0;
        Delay1KTCYx(100);
            WriteSPI(0x9f);
        Delay1KTCYx(100);
            getsSPI(cmd->data, 3);
            cmd->status = 0;
            cmd->size = 3;
            CS_LATCH = 1;
        Delay1KTCYx(100);

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

// Data arriving from USB. All messages are padded to 70 bytes.
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
