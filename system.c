#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "system.h"

#pragma config CPUDIV = NOCLKDIV
#pragma config USBDIV = OFF

#pragma config FOSC = HS
#pragma config PLLEN = ON
#pragma config PCLKEN = ON
#pragma config FCMEN = OFF
#pragma config IESO = OFF

#pragma config PWRTEN = OFF
#pragma config BOREN = OFF
#pragma config BORV = 30

#pragma config WDTEN = OFF
#pragma config WDTPS = 32768

#pragma config HFOFST = OFF
#pragma config MCLRE = OFF

#pragma config DEBUG = ON // BKBUG
#pragma config STVREN = ON
#pragma config LVP = OFF
#pragma config XINST = OFF

#pragma config CP0 = OFF
#pragma config CP1 = OFF

#pragma config CPB = OFF
#pragma config CPD = OFF

#pragma config WRT0 = OFF
#pragma config WRT1 = OFF

#pragma config WRTB = OFF
#pragma config WRTC = OFF
#pragma config WRTD = OFF

#pragma config EBTR0 = OFF
#pragma config EBTR1 = OFF


void ConfigureOscillator(void) {
}
