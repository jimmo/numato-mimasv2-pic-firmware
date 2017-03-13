#!/usr/bin/python3

import serial
import struct
import argparse

SPI_COMMAND_PROGRAMMING_MODE = 1
SPI_COMMAND_GET_FLASH_ID = 2
SPI_COMMAND_ERASE = 3
SPI_COMMAND_PAGE_WRITE_START = 4
SPI_COMMAND_PAGE_WRITE_END = 5
SPI_COMMAND_PAGE_WRITE_DATA = 6

parser = argparse.ArgumentParser(description="Program a PIC")
parser.add_argument('--uart', help="Path to UART device", default='/dev/ttyACM0')
parser.add_argument('--baudrate', help="Baudrate of UART device", default=115200, type=int)
parser.add_argument('--filename', help="Full path to binary file", default='build/mimasv2_base_lm32/flash.bin')
parser.add_argument('--force', help="USe force to continue without confirmation", action='store_true')
args = parser.parse_args()

port = serial.Serial(args.uart, args.baudrate)

def send_command(cmd, data, resp_len):
  packet = struct.pack('BBHB', ord('~'), cmd, len(data), 0) + data
  #print('tx', packet)
  port.write(packet)
  result = port.read(resp_len + 5)
  #print('rx', result)
  _, cmd_reply, len_reply, status_reply = struct.unpack('BBHB', result[0:5])
  if cmd_reply != cmd:
    print('Unexpected reply.')
    return 1, b''
  if len_reply != resp_len:
    print('Unexpected reply length.')
    return 1, b''
  if status_reply != 0:
    print('Error')
  return status_reply, result[5:]

def programming_mode(enable):
  if enable:
    send_command(SPI_COMMAND_PROGRAMMING_MODE, struct.pack('B', 1), 0)
  else:
    send_command(SPI_COMMAND_PROGRAMMING_MODE, struct.pack('B', 0), 0)

def get_spi_flash_id():
  status, flash_id = send_command(SPI_COMMAND_GET_FLASH_ID, b'', 3)
  return flash_id

def erase_spi_flash(image_size):
  image_size |= 0xffff
  send_command(SPI_COMMAND_ERASE, struct.pack('i', image_size), 0)

def page_write_start(addr):
  send_command(SPI_COMMAND_PAGE_WRITE_START, struct.pack('i', addr), 0)

def page_write_end():
  send_command(SPI_COMMAND_PAGE_WRITE_END, b'', 0)

def page_write_data(data):
  if len(data) < 0x100:
    data += b'\x00' * (0x100-len(data))
  write_size = 24
  for i in range(0, 0x100, write_size):
    send_command(SPI_COMMAND_PAGE_WRITE_DATA, data[i:i+write_size], 0)

programming_mode(True)
while True:
  if get_spi_flash_id() == b'\x20\x20\x15':
    print('Found Micron M25P16.')
  else:
    print('Invalid/unknown flash id.')
    break

  try:
    with open(args.filename, 'rb') as image:
      image.seek(0, 2)
      image_size = image.tell()
      image.seek(0, 0)
      image_data = image.read(image_size)
  except (FileNotFoundError, PermissionError):
    print("Could not access file at", args.filename)
    break

  if not args.force:
      while input("Type yes to continue or ctrl+C to abort: ") != "yes":
          pass

  print('Erasing flash...')
  erase_spi_flash(image_size)

  addr = 0
  while addr < image_size:
    print('Programming flash...0x{:08x} {:02d}%\r'.format(addr, 100*addr//image_size), end='')
    page_write_start(addr)
    page_write_data(image_data[addr:addr+0x100])
    page_write_end()
    addr += 0x100
  print('Programming flash...0x{:08x} {:02d}%'.format(addr, 100*addr//image_size))

  break

programming_mode(False)

port.close()
