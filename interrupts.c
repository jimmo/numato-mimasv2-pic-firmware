#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "usb.h"
#include "system.h"

// When using the HID bootloader, you must specify --codeoffset=0x1000 on the
// linker command line.
// Project properties > XC8 linker > Additional options > Code offset

void interrupt high_isr(void) {
#if defined(USB_INTERRUPT)
    if (PIR2bits.USBIF) {
        USBDeviceTasks();
        PIR2bits.USBIF = 0;
    }
#endif
    if (TMR1IE && TMR1IF)
    {
        TMR1IF=0;
        ++tick_count;
    }
}

void low_priority interrupt low_isr(void) {
}
