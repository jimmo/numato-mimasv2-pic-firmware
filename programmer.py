import serial
import struct

port = serial.Serial('/dev/ttyACM0', 115200)

def send_command(cmd, data, resp_len):
  port.write(struct.pack('BBHB', ord('~'), cmd, len(data), 0) + data)
  result = port.read(resp_len + 5)
  _, cmd_reply, len_reply, status_reply = struct.unpack('BBHB', result[0:5])
  if cmd_reply != cmd:
    print('Unexpected reply.')
    return 1, b''
  if len_reply != resp_len:
    print('Unexpected reply length.')
    return 1, b''
  return status_reply, result[5:]

def programming_mode(enable):
  if enable:
    send_command(1, struct.pack('B', 1), 0)
  else:
    send_command(1, struct.pack('B', 0), 0)

def get_spi_flash_id():
  status, flash_id = send_command(2, b'', 3)
  return flash_id

programming_mode(True)
if get_spi_flash_id() == b'\x20\x20\x15':
  print('Found Micron M25P16.')
else:
  print('Invalid/unknown flash id.')
programming_mode(False)

port.close()
