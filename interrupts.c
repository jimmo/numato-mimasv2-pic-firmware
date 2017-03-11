#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "usb.h"



void interrupt high_isr(void) {
#if defined(USB_INTERRUPT)
    USBDeviceTasks();
#endif
}

void low_priority interrupt low_isr(void) {
}
