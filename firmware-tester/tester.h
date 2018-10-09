#define ALL_TESTS -1

enum tests {
  MEMORY_TEST = 0,
  VIDEO_TEST,
  LED_TEST,
  SDCARD_TEST,
  MAX_TESTS
};

#define VIDEO_LFSR_SEED 1

int test_board(int test_number);
unsigned int lfsr_next(void);
void lfsr_init(int seed);
