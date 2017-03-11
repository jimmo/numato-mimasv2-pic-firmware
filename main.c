#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "system.h"

#include "usb.h"
#include "usb_device.h"
#include "usb_device_cdc.h"

void echo(uint8_t cdc_port) {
    uint8_t b;
    uint8_t bytes_read = getsUSBUSART(&b, 1);

    if (bytes_read == 0) {
        return;
    }

    if (b != 0x0a && b != 0x0d) {
        b += cdc_port;
    }

    putUSBUSART(&b, 1);
}

void EchoTask(void) {
    if( USBGetDeviceState() < CONFIGURED_STATE ) {
        return;
    }

    if( USBIsDeviceSuspended()) {
        return;
    }

    if( USBUSARTIsTxTrfReady()) {
        echo(0);
    }

    //if( USBUSARTIsTxTrfReady(1)) {
    //    echo(1);
    //}

    CDCTxService();
}

void main(void) {
    ConfigureOscillator();

    USBDeviceInit();
    USBDeviceAttach();

    while(1) {
#if defined(USB_POLLING)
        USBDeviceTasks();
#endif
        EchoTask();
    }
}

