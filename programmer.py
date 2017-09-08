import serial
import xmodem
import sys

port = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
port.write(b'\r\n\r\n')
if port.in_waiting:
  port.read(port.in_waiting)
port.write(b'i\r')
print(port.read(100))

def xmodem_getc(size, timeout=1):
  r = port.read(size) or None
  #print('Rx:', r)
  return r

def xmodem_putc(data, timeout=1):
  #print('Tx:', data)
  return port.write(data)

modem = xmodem.XMODEM(xmodem_getc, xmodem_putc)
port.write(b'f\r')
port.read(3)

stream = open(sys.argv[1], 'rb')
modem.send(stream)
