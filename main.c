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

    uint8_t buf[100];
    uint8_t bytes_read = getsUSBUSART(cdc_port, buf, sizeof(buf));

    if (bytes_read == 0) {
        return;
    }

    for (uint8_t i = 0; i < bytes_read; ++i) {
        if (buf[i] != 0x0a && buf[i] != 0x0d) {
            buf[i] += cdc_port;
        }
    }

    putUSBUSART(cdc_port, buf, bytes_read);
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

