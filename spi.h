#ifndef PLIB_SPI_H
#define PLIB_SPI_H

#include <xc.h>

void CloseSPI(void);
unsigned char DataRdySPI(void);
void getsSPI( unsigned char *rdptr, unsigned char length );
void OpenSPI( unsigned char sync_mode, unsigned char bus_mode, unsigned char smp_phase);
void putsSPI( unsigned char *wrptr );
void putbufSPI( unsigned char *wrptr, unsigned char length );
unsigned char ReadSPI( void );
signed char WriteSPI( unsigned char data_out );

#endif