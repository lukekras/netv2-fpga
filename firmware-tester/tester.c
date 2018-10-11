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

#include "sdcard.h"

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

  ///////////// CEC TEST
  int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};
  for( i = 0; i < 100; i++ ) { // this loop toggles around 2.1MHz
    val = i & 0x1;
    looptest_cec_tx_write(val);
    if( val ) {
      if( looptest_cec_rx_read() != 0x3 ) {
	syndrome[1] = looptest_cec_rx_read();
	mini_ret++;
      }
    } else {
      if( looptest_cec_rx_read() != 0x0 ) {
	syndrome[0] = looptest_cec_rx_read();
	mini_ret++;
      }
    }
  }
  if( mini_ret != 0 ) {
    printf( "  ERROR: CEC connectivity problem, syndrome: high-%d low-%d, reps: %d\n", syndrome[1], syndrome[0], mini_ret );
    result++;
  }
  looptest_cec_tx_write(0); // return to default value after test
  
  /////////////// HPD override test 
  looptest_rx_forceunplug_write(1);
  if(hdmi_in0_edid_hpd_notif_read()) {
    result++;
    printf( "  ERROR: HPD forceunplug problem\n" );
  }
  looptest_rx_forceunplug_write(0);

  if( result == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  
  return result;
}

#define MEM_TEST_START  0x1000000
#define MEM_TEST_LENGTH 0x0800000
int test_memory(void);
/*
  A very quick memory test. Objective is to find gross solder faults, so:
    - stuck high/low or open address, data and control bits
    - major faults in termination or VTT (minor problems might be missed by this test)

  Program code runs out of low RAM, so that checks if the MSB can toggle.
  The running of code + early memory cal sweep and checks generally catch 
  more subtle errors as well. 

  So, just do a log sweep of 256MiB with a random LFSR pattern that spans one row width
  multiplied by number of banks, and check readback, to stimulate all address bits.

  Note: turns out a comprehensive RAM sweep (fill with random values + readback)
  takes a couple minutes to run, which is too expensive for the quick test
  on the factory floor. If we get to this being an issue to validate on every board,
  consider pulling in proper memory tester code (eg memtester86) which truly works
  all the corner cases.
 */

#define MEM_BANKS 8
#define MEM_ROWS  14
int test_memory(void) {
  int i, j;
  unsigned int *mem;
  unsigned int res = 0;

  printf( "RAM test: " );
  
  unsigned int val;
  mem = (unsigned int *) (MAIN_RAM_BASE + MEM_TEST_START);
  i = 0;
  lfsr_init(0xbabe);
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    for( j = 0; j < (1 << MEM_ROWS) * MEM_BANKS; j++ ) {
      mem[(1 << i) + j] = val;
    }
    i++;
  }
  flush_l2_cache();

  i = 0;
  lfsr_init(0xbabe);
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    for( j = 0; j < (1 << MEM_ROWS) * MEM_BANKS; j++ ) {
      if( mem[(1 << i) + j] != val ) {
	if( res < ERR_PRINT_LIMIT )
	  printf("  ERROR: RAM error at %x, got %x expected %x\n", &(mem[(1 << i) + j]), mem[(1 << i) + j], val );
	res++;
      }
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

/*
  Visual LED test -- requires operator intervention to witness if the LEDs flash
 */
int test_leds(void) {
  int res = 0;
  int last_event;
  int i;
  
  printf( "LED test, please observe all LEDs blinking: " );
  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/8);

  for( i = 0; i < 21; i++ ) {
    while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4) )
      ;
    looptest_leds_write(1 << (i % 3));  
  }
  looptest_leds_write(0);  

  // can't really generate an error code, it's a visual test...so this is more of a "unclear" rather than "PASS/FAIL"
  printf( "FINISHED\n" );
  return res;
}

/*
  Fan test -- requires operator intervention to witness if the fan stops rotating
 */
int test_fan(void) {
  int res = 0;
  int last_event;
  int i;
  
  printf( "Fan test, please observe if the fan stops spinning: " );
  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY);

  for( i = 0; i < 8; i++ ) {
    looptest_fan_pwm_write(i & 1);
    if( i & 1 ) {
      printf( "spinning " );
    } else {
      printf( "stopped " );
    }
    while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
      ;
  }
  looptest_fan_pwm_write(1);  

  // can't really generate an error code, it's a visual test...so this is more of a "unclear" rather than "PASS/FAIL"
  printf( "FINISHED\n" );
  return res;
}

int test_sdcard(void) {
  int res = 0;

  // sd clock defaults to 1MHz in this implementation, don't call frequency init...
  printf( "init sd card\n" );
  sdcard_init();
  //  printf( "one test loop\n" );
  //  sdcard_test(1);
  
  return res;
}

// common kernel function for simple loopbacks
int loopback_kernel( void (*tx_func)(unsigned int), int txbit, unsigned int (*rx_func)(void), int rxbit, char *name ) {
  int res = 0;
  int i;
  unsigned int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};
  for( i = 0; i < 100; i++ ) {
    val = i & 0x1;
    tx_func((unsigned int) (val << txbit));
    if( (rx_func() & (1 << rxbit)) != (val << rxbit) ) {
      syndrome[val] = rx_func();
      mini_ret++;
    }
  }
  if( mini_ret != 0 ) {
    printf( "  ERROR: %s connectivity problem, syndrome: high-0x%x low-0x%x, reps: %d\n", name, syndrome[1], syndrome[0], mini_ret );
    res++;
  }
  tx_func(0);
  return res;
}

/*
  Simple loopback test -- assumes loopback cable connected (short USB D+ to D-)
 */
int test_usb(void) {
  int res = 0;

  printf( "USB test: " );
  res += loopback_kernel( looptest_fusb_tx_write, 0, looptest_fusb_rx_read, 0, "USB" );
  
  if( res == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  return res;
}

/*
  Simple loopback test of low-frequency signals -- requires plug-in to PCIe loopback slot, and MCUINT loopback
 */
int test_loopback(void) {
  int res = 0;

  printf( "Loopback tests: " );

  res += loopback_kernel( looptest_mcu_tx_write, 0, looptest_mcu_rx_read, 0, "MCUINT" );
  res += loopback_kernel( looptest_sm_tx_write, 0, looptest_sm_rx_read, 0, "SM" );

  // this requires a particular wiring on the PCIe test connector, note order
  res += loopback_kernel( looptest_hax_tx_write, 8, looptest_hax_rx_read, 7, "HAX8->7" );
  res += loopback_kernel( looptest_hax_tx_write, 1, looptest_hax_rx_read, 4, "HAX1->4" );
  res += loopback_kernel( looptest_hax_tx_write, 9, looptest_hax_rx_read, 3, "HAX9->3" );
  res += loopback_kernel( looptest_hax_tx_write, 0, looptest_hax_rx_read, 2, "HAX0->2" );
  res += loopback_kernel( looptest_hax_tx_write, 6, looptest_pcie_rx_read, 1, "HAX6->WAKE" );
  res += loopback_kernel( looptest_hax_tx_write, 5, looptest_pcie_rx_read, 0, "HAX5->PERST" );
  
  if( res == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  return res;
}

/*
  Loopback test of high-speed GTP signals -- requires each GTP link to wire Tx to Rx
 */
int test_gtp(void) {
  int res = 0;

  printf( "GTP tests: " );

  // config for 2^7 PRBS test
  gtp0_tx_prbs_config_write(1);
  gtp0_rx_prbs_config_write(1);

  int i;
  for( i = 0; i < 10; i++ ) {
    //    printf( "gtp0_rx_prbs_errors: %d\n", gtp0_rx_prbs_errors_read() );
    printf( "rx errs: %d\n", gtp0_rx_gtp_prbs_err_read() );
  }
  
  if( res == 0 ) {
    printf( "PASS\n" );
  } else {
    printf( "FAIL\n" );
  }
  return res;
}

int test_board(int test_number) {
  int result = 0;

  if( test_number == MEMORY_TEST || test_number == ALL_TESTS ) {
    result += test_memory();
  }
  if( test_number == VIDEO_TEST || test_number == ALL_TESTS ) {
    result += test_video();
  }
  if( test_number == LED_TEST || test_number == ALL_TESTS ) {
    result += test_leds();
  }
  if( test_number == USB_TEST || test_number == ALL_TESTS ) {
    result += test_usb();
  }
  if( test_number == FAN_TEST || test_number == ALL_TESTS ) {
    result += test_fan();
  }
  if( test_number == LOOPBACK_TEST || test_number == ALL_TESTS ) {
    result += test_loopback();
  }
  if( test_number == GTP_TEST || test_number == ALL_TESTS ) {
    result += test_gtp();
  }
  if( test_number == SDCARD_TEST || test_number == ALL_TESTS ) {
    result += test_sdcard();
  }

  return result;
}
