# PIC firmware for the Numato Mimas v2

## Building instructions:

Get the necessary software:

- PIC development tools
  - [MPLAB.X](http://microchip.com/mplabx)
  - [XC8 compiler](http://www.microchip.com/mplab/compilers) for MPLAB.X  (e.g. [linux version](http://www.microchip.com/mplabxc8linux))
  - [XC8 perhipheral library](http://www.microchip.com/mplab/compilers)  (e.g. [linux version](http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en574970))
- [mphidflash](https://code.google.com/archive/p/mphidflash/)

Open the project in MPLAB.X, select Run>Build.

## Flashing

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
$ ./mphidflash-1.6-linux-64 -w ~/src/github.com/jimmo/numato-mimasv2-pic-firmware/dist/XC8_18F14K50/production/numato-mimasv2-pic-firmware.production.hex
```

Remove the jumper and power-cycle the board.
