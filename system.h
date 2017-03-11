#define SYS_FREQ        48000000L
#define FCY             SYS_FREQ/4

#define FIXED_ADDRESS_MEMORY

#define IN_DATA_BUFFER_ADDRESS_TAG      @0x240
#define OUT_DATA_BUFFER_ADDRESS_TAG     @0x260
#define CONTROL_BUFFER_ADDRESS_TAG       @0x2d0

void ConfigureOscillator(void);
