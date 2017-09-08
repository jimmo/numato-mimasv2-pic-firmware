#define SYS_FREQ        48000000L
#define FCY             SYS_FREQ/4
#define _XTAL_FREQ      12000000L

#include <stdint.h>

extern volatile uint16_t tick_count;

void ConfigureOscillator(void);
