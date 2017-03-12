#include <xc.h>

#include "spi.h"

void CloseSPI(void) {
  SSPCON1bits.SSPEN = 0;
}

unsigned char DataRdySPI(void) {
  if ( SSPSTATbits.BF )
    return ( +1 );                // data in SSPBUF register
  else
    return ( 0 );                 // no data in SSPBUF register
}

void getsSPI( unsigned char *rdptr, unsigned char length )
{
  while ( length )                // stay in loop until length = 0
  {
    *rdptr++ = ReadSPI();         // read a single byte
    length--;                     // reduce string length count by 1
  }
}

void OpenSPI( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase)
{
  SSPSTAT &= 0x3F;                // power on state
  SSPCON1 = 0x00;                 // power on state
  SSPCON1 |= sync_mode;           // select serial mode
  SSPSTAT |= smp_phase;           // select data input sample phase

  switch( bus_mode )
  {
    case 0:                       // SPI bus mode 0,0
      SSPSTATbits.CKE = 1;        // data transmitted on falling edge
      break;
    case 2:                       // SPI bus mode 1,0
      SSPSTATbits.CKE = 1;        // data transmitted on rising edge
      SSPCON1bits.CKP = 1;        // clock idle state high
      break;
    case 3:                       // SPI bus mode 1,1
      SSPCON1bits.CKP = 1;        // clock idle state high
      break;
    default:                      // default SPI bus mode 0,1
      break;
  }

  switch( sync_mode )
  {
    case 4:                       // slave mode w /SS enable
		TRISBbits.TRISB6 = 1;		// define clock pin as input
		TRISCbits.TRISC6 = 1;		// define /SS pin as input
		break;

    case 5:                       // slave mode w/o /SS enable
		TRISBbits.TRISB6 = 1;		// define clock pin as input
		break;

	default:                      // master mode, define clock pin as output
		TRISBbits.TRISB6 = 0;		// define clock pin as output
	 break;
  }
		TRISCbits.TRISC7 = 0;       // define SDO pin as output
		TRISBbits.TRISB4 = 1;       // define SDI pin as input

	SSPCON1bits.SSPEN = 1;
}

void putsSPI( unsigned char *wrptr )
{
  while ( *wrptr )                // test for string null character
  {
     SSPBUF = *wrptr++;           // initiate SPI bus cycle
     while( !SSPSTATbits.BF );    // wait until 'BF' bit is set
  }
}

void putbufSPI( unsigned char *wrptr, unsigned char length )
{
  while ( length )                // test for string null character
  {
     SSPBUF = *wrptr++;           // initiate SPI bus cycle
     while( !SSPSTATbits.BF );    // wait until 'BF' bit is set
     --length;
  }
}

unsigned char ReadSPI( void )
{
  unsigned char TempVar;
  TempVar = SSPBUF;        // Clear BF
  PIR1bits.SSPIF = 0;      // Clear interrupt flag
  SSPBUF = 0x00;           // initiate bus cycle
  //while ( !SSPSTATbits.BF );                  // wait until cycle complete
  while(!PIR1bits.SSPIF);  // wait until cycle complete
  return ( SSPBUF );       // return with byte read
}

signed char WriteSPI( unsigned char data_out )
{
  unsigned char TempVar;
  TempVar = SSPBUF;           // Clears BF
  PIR1bits.SSPIF = 0;         // Clear interrupt flag
  SSPCON1bits.WCOL = 0;			//Clear any previous write collision
  SSPBUF = data_out;           // write byte to SSPBUF register
  if ( SSPCON1 & 0x80 )        // test if write collision occurred
   return ( -1 );              // if WCOL bit is set return negative #
  else
   // while( !SSPSTATbits.BF );  // wait until bus cycle complete
    while( !PIR1bits.SSPIF );  // wait until bus cycle complete
   return ( 0 );                // if WCOL bit is not set return non-negative#
}
