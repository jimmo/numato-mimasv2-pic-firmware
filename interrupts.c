#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "usb.h"

// When using the HID bootloader, you must specify --codeoffset=0x1000 on the
// linker command line.
// Project properties > XC8 linker > Additional options > Code offset

void interrupt high_isr(void) {
#if defined(USB_INTERRUPT)
    USBDeviceTasks();
#endif
}

void low_priority interrupt low_isr(void) {
}
