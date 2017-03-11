#include <xc.h>

#include <stdint.h>
#include <stdbool.h>

#include "system.h"

#include "usb.h"
#include "usb_device.h"
#include "usb_device_cdc.h"

#include "fpga_uart.h"
#include "spi_flash.h"

void EchoTask(uint8_t cdc_port) {
    if (!USBUSARTIsTxTrfReady(cdc_port)) {
        return;
    }

    uint8_t b;
    uint8_t bytes_read = getsUSBUSART(cdc_port, &b, 1);

    if (bytes_read == 0) {
        return;
    }

    if (b != 0x0a && b != 0x0d) {
        b += cdc_port;
    }

    putUSBUSART(cdc_port, &b, 1);
}

void main(void) {
    ConfigureOscillator();

    USBDeviceInit();
    USBDeviceAttach();

    SpiFlashInit();
    FpgaUartInit();

    while(1) {
#if defined(USB_POLLING)
        USBDeviceTasks();
#endif

        if( USBGetDeviceState() < CONFIGURED_STATE || USBIsDeviceSuspended()) {
            continue;
        }

        SpiFlashTask();
        FpgaUartTask();
        CDCTxService();
    }
}

