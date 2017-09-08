import serial
import sys
import xmodem


if len(sys.argv) != 2:
  print('Usage: {} filename.bin'.format(sys.argv[0]))
  sys.exit(1)

port = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
port.write(b'\x03\r\n\r\n')
port.read(100)
port.write(b'i\r')
chipid = port.read(100).decode()
chipid = chipid.split('\n')
if len(chipid) > 1 and chipid[1].strip() == '202015':
  print('Detected Mimas v2 with M25P16 SPI Flash.')
else:
  sys.exit(1)

def xmodem_getc(size, timeout=1):
  port.timeout = timeout
  r = port.read(size) or None
  #print('Rx:', r)
  return r

def xmodem_putc(data, timeout=1):
  port.timeout = timeout
  #print('Tx:', data)
  return port.write(data)

print('Sending {}'.format(sys.argv[1]))
modem = xmodem.XMODEM(xmodem_getc, xmodem_putc, mode='xmodem1k')
port.write(b'f\r')
port.read(3)

stream = open(sys.argv[1], 'rb')
modem.send(stream)
print('Done.')
