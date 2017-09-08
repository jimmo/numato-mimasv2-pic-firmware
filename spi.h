#ifndef PLIB_SPI_H
#define PLIB_SPI_H

#include <xc.h>

#include <stdint.h>

void SpiRead(uint8_t* rdptr, uint8_t length);
void SpiWrite(uint8_t* wrptr, uint8_t length);
uint8_t SpiReadByte(void);
uint8_t SpiWriteByte(uint8_t data_out);

#endif
