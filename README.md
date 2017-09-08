# PIC firmware for the Numato Mimas v2

## Overview

This replaces the factory firmware and addresses two main limitations:

- The FPGA UART now runs at 115200 baud.
- The board shows up as two USB serial ports, one for the SPI Flash programmer, one for the FPGA UART. This avoids the need to use the mode switch on the board.

The programming interface supports XMODEM-CRC and XMODEM1k removing the need for a custom programmer.

## Building instructions:

There's a prebuilt .hex file in the `prebuilt` directory.

Otherwise, get the necessary software:

- PIC development tools
  - [MPLAB.X](http://microchip.com/mplabx)
  - [XC8 compiler](http://www.microchip.com/mplab/compilers) for MPLAB.X  (e.g. [linux version](http://www.microchip.com/mplabxc8linux))
  - [60-day PRO demo](http://www.microchip.com/xcdemo/GetDemoLicense.aspx)
  - [XC8 legacy PIC18F perhipheral library](http://www.microchip.com/mplab/compilers)  (e.g. [linux version](http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en574970)) (not required just to build).
- [mphidflash](https://code.google.com/archive/p/mphidflash/)

Open the project in MPLAB.X, select Run>Build.

## Flashing the PIC

Connect a jumper to the FWUP header (just above the left-most seven-segment). Power up the board and it should show up as a HID device (the Microchip HID bootloader).

```
$ lsusb
...
Bus 001 Device 052: ID 04d8:003c Microchip Technology, Inc.
...
```

Verify mphidflash can find the device:

```
$ ./mphidflash-1.6-linux-64
USB HID device found: 12288 bytes free
Device family: PIC18
```

Use mphidflash to flash the new code:

```
$ ./mphidflash-1.6-linux-64 -w prebuilt/numato-mimasv2-pic-firmware.production.hex -r
```

Remove the jumper and power-cycle the board.

## Flashing the FPGA

### Using screen

```
$ screen /dev/ttyACM0
mimas> v
1.0
mimas> i
202015
mimas> f
Ctrl-A :  exec !! sx -k /path/to/flash.bin
```

### Using programmer.py

```
$ python3 -m venv venv
$ . venv/bin/activate
$ pip install pyserial xmodem
$ python3 programmer.py /path/to/flash.bin
```

### Using minicom

```
$ minicom -D /dev/ttyACM0
mimas> v
1.0
mimas> i
202015
mimas> f
Ctrl-A S
```

## Connecting to the FPGA UART

```
$ screen /dev/ttyACM1 115200
```

## TODO

- Make the FPGA UART speed detect automatically from the host.
