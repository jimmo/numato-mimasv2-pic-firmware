#include <xc.h>

#include "spi.h"

void SpiRead(uint8_t* rdptr, uint8_t length) {
    while (length) {
        *rdptr++ = SpiReadByte(); // read a single byte
        length--; // reduce string length count by 1
    }
}

void SpiWrite(uint8_t* wrptr, uint8_t length) {
    while (length) {
        SSPBUF = *wrptr++; // initiate SPI bus cycle
        while (!SSPSTATbits.BF); // wait until 'BF' bit is set
        --length;
    }
}

uint8_t SpiReadByte(void) {
    unsigned char TempVar;
    TempVar = SSPBUF; // Clear BF
    PIR1bits.SSPIF = 0; // Clear interrupt flag
    SSPBUF = 0x00; // initiate bus cycle
    //while ( !SSPSTATbits.BF );                  // wait until cycle complete
    while (!PIR1bits.SSPIF); // wait until cycle complete
    return ( SSPBUF); // return with byte read
}

uint8_t SpiWriteByte(uint8_t data_out) {
    unsigned char TempVar;
    TempVar = SSPBUF; // Clears BF
    PIR1bits.SSPIF = 0; // Clear interrupt flag
    SSPCON1bits.WCOL = 0; //Clear any previous write collision
    SSPBUF = data_out; // write byte to SSPBUF register
    if (SSPCON1 & 0x80) // test if write collision occurred
        return 0; // if WCOL bit is set return negative #
    else
        // while( !SSPSTATbits.BF );  // wait until bus cycle complete
        while (!PIR1bits.SSPIF); // wait until bus cycle complete
    return 1; // if WCOL bit is not set return non-negative#
}
