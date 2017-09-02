#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>

const int HANDLE_OK = 0;
const int HANDLE_EXIT = 1;
const int HANDLE_RESET = 2;

const char MSG_PROMPT[] = "mimas> ";
const char MSG_COMPLETE[] = "\nxmodem complete.\n";
const char MSG_FAILED[] = "\nxmodem failed.\n";
const char MSG_NEWLINE[] = "\n";

const int XMODEM_RETRIES = 5;

uint8_t buf[140];
const uint8_t* bufend = buf + sizeof(buf);

// Start of Header
const uint8_t XMODEM_SOH = 0x01;

// End of Transmission
const uint8_t XMODEM_EOT = 0x04;

// Acknowledge
const uint8_t XMODEM_ACK = 0x06;

// Not Acknowledge (or start xmodem)
const uint8_t XMODEM_NACK = 0x15;

// End of Transmission Block (Return to Amulet OS mode)
const uint8_t XMODEM_ETB = 0x17;

// Cancel (Force receiver to start sending C's)
const uint8_t XMODEM_CAN = 0x18;

// ASCII “C” (or start xmodem-crc)
const uint8_t XMODEM_C = 0x43;

unsigned short xmodem_crc16(uint8_t* buf, size_t len) {
  const uint16_t poly = 0x1021;
  int16_t crc = 0;

  for(int i = 0; i < len; ++i) {
    uint16_t x = buf[i];
    crc = (crc ^ x << 8);
    for (int j = 0; j < 8; ++j) {
      if (crc & 0x8000) {
        crc = crc << 1 ^ poly;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

void xmodem(void) {
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  int n = 0;

  uint8_t* next = buf;
  write(STDOUT_FILENO, &XMODEM_C, 1);

  uint8_t seq = 1;
  while (1) {
    ssize_t len = read(STDIN_FILENO, next, bufend - next);
    if (len < 0) {
      struct timespec t2;
      clock_gettime(CLOCK_MONOTONIC, &t2);
      if (t2.tv_sec - t1.tv_sec > 1) {
        write(STDOUT_FILENO, &XMODEM_C, 1);
        t1 = t2;
        ++n;
        if (n > XMODEM_RETRIES) {
          write(STDIN_FILENO, MSG_FAILED, strlen(MSG_FAILED));
          return;
        }
      }
      continue;
    }
    next += len;
    if (next > buf) {
      if (*buf == XMODEM_SOH) {
        // Start of frame -- see if we have the full frame.
        if (next - buf >= (1 + 2 + 128 + 2)) {
          uint16_t crc = buf[1 + 2 + 128];
          crc = crc << 8 | buf[1 + 2 + 128 + 1];
          if (buf[1] == 255 - buf[2] && seq == buf[1] && xmodem_crc16(buf + 1 + 2, 128) == crc) {
            write(STDOUT_FILENO, &XMODEM_ACK, 1);
            seq += 1;
          } else {
            write(STDOUT_FILENO, &XMODEM_NACK, 1);
          }
          next = buf;
          continue;
        }
      } else if (*buf == XMODEM_EOT) {
        // End of file.
        write(STDOUT_FILENO, &XMODEM_ACK, 1);
        write(STDIN_FILENO, MSG_COMPLETE, strlen(MSG_COMPLETE));
        return;
      } else {
        // Frame started with something else.
        write(STDOUT_FILENO, &XMODEM_NACK, 1);
        next = buf;
        continue;
      }
    }
  }
}

int handle(uint8_t* cmd, ssize_t len) {
  if (strcmp(cmd, "exit") == 0) {
    return HANDLE_EXIT;
  }
  if (strcmp(cmd, "xmodem") == 0) {
    xmodem();
    return HANDLE_RESET;
  }
  return HANDLE_OK;
}

int prompt(void) {
  uint8_t* bufend = buf + sizeof(buf);
  uint8_t* next = buf;

  write(STDOUT_FILENO, MSG_PROMPT, strlen(MSG_PROMPT));
  while (1) {

    // aaaaaaaaaaaaaaa\nbbbbbbbbbbb\ncccccccccc
    // ^ buf   ^next  ^ end

    ssize_t len = read(STDIN_FILENO, next, bufend - next);
    if (len < 0) {
      continue;
    }
    uint8_t* end = NULL;
    while (end = memchr(next, '\n', len)) {
      write(STDOUT_FILENO, next, end + 1 - next);
      *end = 0;
      int r = handle(buf, end - buf);
      if (r == HANDLE_EXIT) {
        return 0;
      } else if (r == HANDLE_RESET) {
        return 1;
      }
      write(STDOUT_FILENO, MSG_PROMPT, strlen(MSG_PROMPT));

      memmove(buf, end + 1, bufend - (end + 1));
      len -= (end + 1) - next;
      next = buf;
    }

    write(STDOUT_FILENO, next, len);
    next += len;
  }

  return 0;
}

int main(int argc, char** argv) {
  struct termios ctrl, ctrl_reset;
  tcgetattr(STDIN_FILENO, &ctrl);
  ctrl_reset = ctrl;
  ctrl.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &ctrl);

  fcntl(STDIN_FILENO, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);

  while (prompt());

  tcsetattr(STDIN_FILENO, TCSANOW, &ctrl_reset);
  return 0;
}
