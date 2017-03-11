#ifndef FPGA_UART_H
#define	FPGA_UART_H

#include <xc.h> // include processor files - each processor file is guarded.

void FpgaUartInit(void);

void FpgaUartTask(void);

#endif
