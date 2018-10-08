#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <generated/csr.h>
#include <time.h>

#include "tester.h"
#include "hdmi_in0.h"
#include "hdmi_in1.h"
#include "pattern.h"
#include "hdmi_out0.h"
#include "hdmi_out1.h"

static int lfsr_state = 1;
void lfsr_init(int seed) {
  lfsr_state = seed;
}

unsigned int lfsr_next(void)
{
  /*
    config          : galois
    length          : 32
    taps            : (32, 25, 17, 7)
    shift-amount    : 16
    shift-direction : right
  */
  enum {
    length = 32,
    tap_0  = 32,
    tap_1  = 25,
    tap_2  = 17,
    tap_3  =  7
  };
  int v = lfsr_state;
  typedef unsigned int T;
  const T zero = (T)(0);
  const T lsb = zero + (T)(1);
  const T feedback = (
		      (lsb << (tap_0 - 1)) ^
		      (lsb << (tap_1 - 1)) ^
		      (lsb << (tap_2 - 1)) ^
		      (lsb << (tap_3 - 1))
		      );
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  v = (v >> 1) ^ ((zero - (v & lsb)) & feedback);
  lfsr_state = v;
  return v;
}


#define VIDEO_HACTIVE  1280
#define VIDEO_VACTIVE  720
#define VIDEO_FREQ     74250000

#define ERR_PRINT_LIMIT 100
int transform_source(int source);
int transform_source(int source) {
  return (source & 0xFF) << 16 | (source & 0xFF0000) >> 16 | (source & 0x00FF00);
}

extern unsigned char netv_edid_60hz[256];

int test_video(void);
/* 
   Hardware configuration:
     TX1 connected to RX0
     TX0 connected to OVERLAY
     Jumpers in SOURCE position
 */
int test_video(void) {
  int result = 0;
  int resdiff;
  int num_err_printed = 0;
  int i;
  
  printf( "video test: " );

  /////////// PLL TEST
  resdiff = result;
  if( hdmi_in0_resdetection_hres_read() != VIDEO_HACTIVE )
    result++;
  if( hdmi_in0_resdetection_vres_read() != VIDEO_VACTIVE )
    result++;
  if( hdmi_in0_freq_value_read() != VIDEO_FREQ )
    result++;
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in0 res/freq wrong\n" );
    num_err_printed++;
  }

  resdiff = result;
  if( hdmi_in1_resdetection_hres_read() != VIDEO_HACTIVE )
    result++;
  if( hdmi_in1_resdetection_vres_read() != VIDEO_VACTIVE )
    result++;
  if( hdmi_in1_freq_value_read() != VIDEO_FREQ )
    result++;
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in1 res/freq wrong\n" );
    num_err_printed++;
  }

  /////////// DATA LINK TEST
  unsigned int *framebuffer = (unsigned int *)(MAIN_RAM_BASE + hdmi_in0_framebuffer_base(0));
  unsigned int expected;
  int last_event;

  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4); // initialize the last_event time variable
  
  resdiff = result;
  // clear the framebuffer so we aren't testing stale data
  for(i=1; i<VIDEO_HACTIVE*VIDEO_VACTIVE*2/4; i++) {
    framebuffer[i] = 0;
  }
  pattern_fill_framebuffer_test(VIDEO_VACTIVE, VIDEO_HACTIVE, 1);
  while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4) )
    ;
  
  lfsr_init(1);
  for(i=0; i<VIDEO_HACTIVE*VIDEO_VACTIVE*2/4; i++) {
    expected = transform_source(lfsr_next());
    if( framebuffer[i] != expected ) {
      result++;
      if( num_err_printed < ERR_PRINT_LIMIT ) {
	printf( "  ERROR: hdmi_in0 mismatch at %d: expect %x, got %x\n", i, expected, framebuffer[i] );
	num_err_printed++;
      }
    }
  }
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in0 data corruption\n" );
    num_err_printed++;
  }

  resdiff = result;
  framebuffer = (unsigned int *)(MAIN_RAM_BASE + hdmi_in1_framebuffer_base(0));
  // clear the framebuffer so we aren't testing stale data
  for(i=0; i<VIDEO_HACTIVE*VIDEO_VACTIVE*2/4; i++) {
    framebuffer[i] = 0;
  }
  pattern_fill_framebuffer_test(VIDEO_VACTIVE, VIDEO_HACTIVE, 2);
  while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4) )
    ;
  
  lfsr_init(2);
  for(i=0; i<VIDEO_HACTIVE*VIDEO_VACTIVE*2/4; i++) {
    expected = transform_source(lfsr_next());
    if( framebuffer[i] != expected ) {
      result++;
      if( num_err_printed < ERR_PRINT_LIMIT ) {
	printf( "  ERROR: hdmi_in1 mismatch at %d: expect %x, got %x\n", i, expected, framebuffer[i] );
	num_err_printed++;
      }
    }
  }
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in1 data corruption\n" );
    num_err_printed++;
  }

  /////////// EDID/DDC TEST
  // EDID TEST ESCAPES:
  // RX0 SDA override high is not tested
  unsigned char edid[256];
  resdiff = result;
  result += hdmi_out0_read_edid(edid);
  for( i = 0; i < 256; i++ ) {
    if( edid[i] != netv_edid_60hz[i] ) {
      result++;
    }
  }
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in0 DDC bus problem\n" );
  }

  resdiff = result;
  result += hdmi_out1_read_edid(edid);
  for( i = 0; i < 256; i++ ) {
    if( edid[i] != netv_edid_60hz[i] ) {
      result++;
    }
  }
  if( resdiff != result ) {
    printf( "  ERROR: hdmi_in1 DDC bus problem\n" );
  }

  // next up: check CEC, HPD
  

  if( result == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  
  return result;
}

#define MEM_TEST_START 0x1000000
#define MEM_TEST_LENGTH 0x1000000
int test_memory(void);
/*
  A very quick memory test. Objective is to find gross solder faults, so:
    - stuck high/low or open address, data and control bits
    - major faults in termination or VTT (minor problems might be missed by this test)

  Program code runs out of low RAM, so that checks if the MSB can toggle.
  The running of code + early memory cal sweep and checks generally catch 
  more subtle errors as well. 

  So, just do a log sweep of 256MiB with a random LFSR pattern and check 
  readback, to make sure that no high address bits are bad.

  Note: turns out a comprehensive RAM sweep (fill with random values + readback)
  takes a couple minutes to run, which is too expensive for the quick test
  on the factory floor. If we get to this being an issue to validate on every board,
  consider pulling in proper memory tester code (eg memtester86) which truly works
  all the corner cases.
 */

int test_memory(void) {
  int i;
  unsigned int *mem;
  unsigned int res = 0;

  printf( "RAM test: " );
  
  lfsr_init(0xbabe);
  mem = (unsigned int *) (MAIN_RAM_BASE + MEM_TEST_START);
  i = 0;
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    mem[1 << i] = lfsr_next();
    i++;
  }

  lfsr_init(0xbabe);
  unsigned int val;
  i = 0;
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    val = lfsr_next();
    if( mem[1 << i] != val ) {
      if( res < ERR_PRINT_LIMIT )
	printf("  ERROR: RAM error at %x, got %x expected %x\n", i * sizeof(int), mem[i], val );
      res++;
    }
    i++;
  }
  
  if( res == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  return(res);
}

int test_board(int test_number) {
  int result = 0;

  if( test_number == MEMORY_TEST || test_number == ALL_TESTS ) {
    result += test_memory();
  }
  if( test_number == VIDEO_TEST || test_number == ALL_TESTS ) {
    result += test_video();
  }

  return result;
}
