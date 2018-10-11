#define ALL_TESTS -1

enum tests {
  MEMORY_TEST = 0,
  VIDEO_TEST,
  LED_TEST,
  SDCARD_TEST,
  USB_TEST,
  FAN_TEST,
  LOOPBACK_TEST,
  GTP_TEST,
  XADC_TEST,
  MAX_TESTS
};

#define VIDEO_LFSR_SEED 1

int test_board(int test_number);
unsigned int lfsr_next(void);
void lfsr_init(int seed);

int test_video(void);
int test_memory(void);
int test_fan(void);
int test_sdcard(void);
int test_usb(void);
int test_loopback(void);
int test_gtp(void);
int test_leds(void);
int test_xadc(void);
