#include <xc.h>

#include <stdint.h>

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


char USB_Out_Buffer[CDC_DATA_OUT_EP_SIZE];
PTR_SPARTAN_3A_CONFIG_IN_PACKET PtrRespPacket = (PTR_SPARTAN_3A_CONFIG_IN_PACKET) USB_Out_Buffer;

void SPIOpen(unsigned char SyncMode, unsigned char BusMode, unsigned char SmpPhase);

void SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN Pin, unsigned char Direction);
void SPISetIOValue(SPARTAN_3A_CONFIG_IO_PIN Pin, unsigned char Value);
unsigned char SPIGetIOValue(SPARTAN_3A_CONFIG_IO_PIN Pin);


void ServicePacket(PTR_SPARTAN_3A_CONFIG_OUT_PACKET ptrPacket);
unsigned char USBSendPacket(unsigned char* data, unsigned char Size);
void USBSendStatus(SPARTAN_3A_CONFIG_STATUS Status, uint8_t SPINum, uint8_t ExecutedCmd);

void SPIOpen(unsigned char SyncMode, unsigned char BusMode, unsigned char SmpPhase)
{
		//OpenSPI(SyncMode, BusMode, SmpPhase);

		//Set SDO SPI Pin directions
		ANSELHbits.ANS9 = 0;// RC7 as digital IO
		ANSELHbits.ANS10 = 0;// RB4 as digital IO

		TRISCbits.TRISC7 = 0;
		TRISBbits.TRISB4 = 1;
		TRISBbits.TRISB6 = 0;

		//SSPSTAT settings
		SSPSTATbits.SMP = 0; //Sample at middle
		SSPSTATbits.CKE = 1;

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
}

void SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN Pin, unsigned char Direction)
{
		switch(Pin)
		{
			case SPARTAN_3A_CONFIG_IO_PIN_SI:
				SI_TRIS	 = Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_SO:
				SO_TRIS	= Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CS:
				CS_TRIS	 = Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CLK:
				CLK_TRIS = Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_PROGB:
				PROGB_TRIS	= Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_DONE:
				CDONE_TRIS = Direction;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_INITB:
				INITB_TRIS = Direction;
			break;
		}
}

void SPISetIOValue(SPARTAN_3A_CONFIG_IO_PIN Pin, unsigned char Value)
{
		switch(Pin)
		{
			case SPARTAN_3A_CONFIG_IO_PIN_SI:
				SI_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_SO:
				SO_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CS:
				CS_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CLK:
				CLK_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_PROGB:
				PROGB_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_DONE:
				CDONE_LATCH = Value;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_INITB:
				INITB_LATCH = Value;
			break;
		}
}

unsigned char SPIGetIOValue(SPARTAN_3A_CONFIG_IO_PIN Pin)
{
	unsigned char tmpValue = 0xFF;
		switch(Pin)
		{
			case SPARTAN_3A_CONFIG_IO_PIN_SI:
				tmpValue = SI_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_SO:
				tmpValue = SO_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CS:
				tmpValue = CS_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_CLK:
				tmpValue = CLK_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_PROGB:
				tmpValue = PROGB_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_DONE:
				tmpValue = CDONE_PORT;
			break;

			case SPARTAN_3A_CONFIG_IO_PIN_INITB:
				tmpValue = INITB_PORT;
			break;
		}

	return tmpValue;
}

void
Delay1KTCYx(unsigned char unit)
{
	do {
		_delay(1000);
	} while(--unit != 0);
}

void SpiFlashInit(void) {
    //Set all pins except PROGB as inputs
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_SI, IO_DIRECTION_IN);
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_SO, IO_DIRECTION_IN);
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_CS, IO_DIRECTION_IN);
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_CLK, IO_DIRECTION_IN);
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_DONE, IO_DIRECTION_IN);
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_INITB, IO_DIRECTION_IN);

	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_PROGB, IO_DIRECTION_OUT);

	//Pull PROG_B Low for a few milliseconds to start configuration
	SPISetIOValue(SPARTAN_3A_CONFIG_IO_PIN_PROGB, 0);
	Delay1KTCYx(100);
	SPISetIOValue(SPARTAN_3A_CONFIG_IO_PIN_PROGB, 1);

	//Leave PROGB in HiZ state
	SPISetIODirection(SPARTAN_3A_CONFIG_IO_PIN_PROGB, IO_DIRECTION_IN);
}

void ServicePacket(PTR_SPARTAN_3A_CONFIG_OUT_PACKET ptrPacket)
{
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
	else if(ptrPacket->RawPacket.Byte1 == SPARTAN_3A_CONFIG_OUT_PACKET_SPI_PUTSTRING)
	{
		putbufSPI(&ptrPacket->SPIPutString.Data[0], ptrPacket->SPIPutString.Length);
		USBSendStatus(STATUS_SUCCESS, ptrPacket->SetIOValue.SpiNum, ptrPacket->RawPacket.Byte1);
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
	if( Size > CDC_DATA_IN_EP_SIZE)
	{
		//This shouldn't happen ideally.
		//return STATUS_FAIL_PACKET_SIZE_TOO_LARGE;
		Size = CDC_DATA_IN_EP_SIZE;
	}

	putUSBUSART(SPI_CDC_PORT, (char*)data, Size);
    return 0;
}

// Data arriving from USB.
uint8_t usb_rx_buf[CDC_DATA_IN_EP_SIZE] = {0};
uint8_t usb_rx_avail = 0;

void SpiFlashTask(void) {
    uint8_t usb_bytes_read = getsUSBUSART(SPI_CDC_PORT, usb_rx_buf + usb_rx_avail, sizeof(usb_rx_buf) - usb_rx_avail);
    if (usb_bytes_read) {
        usb_rx_avail += usb_bytes_read;
        if (usb_rx_avail == sizeof(usb_rx_buf)) {
            ServicePacket((PTR_SPARTAN_3A_CONFIG_OUT_PACKET)usb_rx_buf);
            usb_rx_avail = 0;
        }
    }
}
