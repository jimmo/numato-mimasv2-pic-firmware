import argparse
import serial
import sys
import xmodem

parser = argparse.ArgumentParser(description='Mimas V2 Programmer')
parser.add_argument('--uart', help='Path to UART device', default='/dev/ttyACM0')
parser.add_argument('--baudrate', help='Baudrate of UART device', default=115200, type=int)
parser.add_argument('--filename', help='Full path to binary file', default='build/mimasv2_base_lm32/flash.bin')
verify_parser = parser.add_mutually_exclusive_group(required=False)
verify_parser.add_argument('--verify', dest='verify', action='store_true')
verify_parser.add_argument('--no-verify', dest='verify', action='store_false')
parser.set_defaults(verify=True)
erase_parser = parser.add_mutually_exclusive_group(required=False)
erase_parser.add_argument('--erase', dest='erase', action='store_true')
erase_parser.add_argument('--no-erase', dest='erase', action='store_false')
parser.set_defaults(erase=False)
parser.add_argument('--protocol', help='XMODEM protocol (xmodem or xmodem1k)', default='xmodem1k')
args = parser.parse_args()

port = serial.Serial(args.uart, args.baudrate, timeout=0.1)

port.write(b'\x03\r\n\r\n')
port.read(100)
port.write(b'i\r')
chipid = port.read(100).decode()
chipid = chipid.split('\n')
if len(chipid) > 1 and chipid[1].strip() == '202015':
  print('Detected Mimas v2 with M25P16 SPI Flash.')
else:
  print('Didn\'t find SPI Flash.')
  sys.exit(1)

if args.erase:
  print('Erasing flash -- this takes approx 16 seconds.')
  port.write(b'e\r')
  while True:
    if port.read(1) == b'>':
      break
  port.read(1)
  print('Erase complete.')

if args.verify:
  port.write(b'C\r')
else:
  print('Warning: disabling verification.')
  port.write(b'c\r')
port.read(len('x\r\nmimas> '))

def xmodem_getc(size, timeout=1):
  port.timeout = timeout
  r = port.read(size) or None
  #print('Rx:', r)
  return r

def xmodem_putc(data, timeout=1):
  port.timeout = timeout
  print('.', end='', flush=True)
  #print('Tx:', data)
  return port.write(data)

print('Sending {}'.format(args.filename))
modem = xmodem.XMODEM(xmodem_getc, xmodem_putc, mode=args.protocol)
port.write(b'f\r')
port.read(3)

with open(args.filename, 'rb') as stream:
  if modem.send(stream):
    print()
    print('Done.')
  else:
    print('Failed.')
