#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <system.h>

#include <generated/csr.h>
#include <time.h>

#include "tester.h"
#include "hdmi_in0.h"
#include "hdmi_in1.h"
#include "pattern.h"
#include "hdmi_out0.h"
#include "hdmi_out1.h"

#include "sdcard.h"

// output validated with https://jsonformatter.curiousconcept.com/
//#define FORCERR   // uncomment to help debug JSON formatting

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
  int firstfail = 1;
  
#ifdef CSR_HDMI_IN0_BASE  
  printf( "  {\n");
  printf( "    \"subtest_name\":\"video\", \n" );
  printf( "    \"failures\":[\n" );

  /////////// PLL TEST
  resdiff = result;
  if( hdmi_in0_resdetection_hres_read() != VIDEO_HACTIVE )
    result++;
  if( hdmi_in0_resdetection_vres_read() != VIDEO_VACTIVE )
    result++;
  if( hdmi_in0_freq_value_read() != VIDEO_FREQ )
    result++;
  if( resdiff != result ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"%s\",\n", "HDMI in0 res/freq" );
    printf( "     \"syndrome\": {\"hdmi_in0_hres\":%d, ", hdmi_in0_resdetection_hres_read() );
    printf( "\"hdmi_in0_vres\":%d, ", hdmi_in0_resdetection_vres_read() );
    printf( "\"hdmi_in0_freq\":%d }}", hdmi_in0_freq_value_read() );
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
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"%s\",\n", "HDMI in1 res/freq" );
    printf( "     \"syndrome\": {\"hdmi_in1_hres\":%d, ", hdmi_in1_resdetection_hres_read() );
    printf( "\"hdmi_in1_vres\":%d, ", hdmi_in1_resdetection_vres_read() );
    printf( "\"hdmi_in1_freq\":%d }}", hdmi_in1_freq_value_read() );
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
	if( firstfail ) {
	  firstfail = 0;
	} else {
	  printf( ",\n" );
	}
	printf( "    {\"name\":\"%s\",\n", "HDMI in0 link" );
	printf( "     \"syndrome\": {\"hdmi_in0\":\"mismatch\", \"offset\":%d, \"got\":%d, \"expected\":%d}}", i, expected, framebuffer[i] );
	num_err_printed++;
      }
    }
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
#ifdef FORCERR
    expected++;
#endif
    if( framebuffer[i] != expected ) {
      result++;
      if( num_err_printed < ERR_PRINT_LIMIT ) {
	if( firstfail ) {
	  firstfail = 0;
	} else {
	  printf( ",\n" );
	}
	printf( "    {\"name\":\"%s\",\n", "HDMI in1 link" );
	printf( "     \"syndrome\": {\"hdmi_in1\":\"mismatch\", \"offset\":%d, \"got\":%d, \"expected\":%d}}", i, expected, framebuffer[i] );
	num_err_printed++;
      }
    }
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
#ifdef FORCERR
    result++;
#endif
  if( resdiff != result ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"hdmi_in0 DDC\"}" );
  }

  resdiff = result;
  result += hdmi_out1_read_edid(edid);
  for( i = 0; i < 256; i++ ) {
    if( edid[i] != netv_edid_60hz[i] ) {
      result++;
    }
  }
#ifdef FORCERR
    result++;
#endif
  if( resdiff != result ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"hdmi_in1 DDC\"}" );
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
#ifdef FORCERR
    mini_ret++;
#endif
  if( mini_ret != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"%s\",\n", "CEC" );
    printf( "     \"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}", syndrome[1], syndrome[0], mini_ret );
    result++;
  }
  looptest_cec_tx_write(0); // return to default value after test
  
  /////////////// HPD override test 
  looptest_rx_forceunplug_write(1);
#ifdef FORCERR
  if(1) {
#else
  if(hdmi_in0_edid_hpd_notif_read()) {
#endif
    result++;
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"HPD force unplug\"}" );
  }
  looptest_rx_forceunplug_write(0);

  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", result );
#endif  
  return result;
}

#define MEM_TEST_START  0x08000000
#define MEM_TEST_LENGTH 0x04000000
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
#define MEM_BANKS_LOG  3
#define MEM_ROWS  14
int test_memory(void) {
  unsigned int i, j;
  unsigned int *mem;
  unsigned int res = 0;
  int firstfail = 1;
  
  printf( "  {\n");
  printf( "    \"subtest_name\":\"RAM\", \n" );
  printf( "    \"failures\":[\n" );

  unsigned int row_end = ((1 << MEM_ROWS) * MEM_BANKS) / sizeof(int);
  unsigned int val;
  mem = (unsigned int *) (MAIN_RAM_BASE + MEM_TEST_START);
  i = 0;
  lfsr_init(0xbabe);
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    for( j = 0; j < row_end; j++ ) {
      val = lfsr_next();
      mem[(1 << i) + j] = val;
    }
    if( i == 0 )
      i += (MEM_ROWS + MEM_BANKS_LOG);
    else
      i++;
  }
  flush_l2_cache();

  i = 0;
  lfsr_init(0xbabe);
  while( (1 << i) < (MEM_TEST_LENGTH / sizeof(int) ) ) {
    for( j = 0; j < row_end; j++ ) {
      val = lfsr_next();
#ifdef FORCERR
      val++;
#endif
      if( mem[(1 << i) + j] != val ) {
	if( res < ERR_PRINT_LIMIT ) {
	  if( firstfail ) {
	    firstfail = 0;
	  } else {
	    printf( ",\n" );
	  }
	  printf( "    {\"name\":\"%s\",\n", "RAM" );
	  printf( "     \"syndrome\": {\"addr\":%d, \"got\":%d, \"expected\":%d}}", &(mem[(1 << i) + j]), mem[(1 << i) + j], val );
	}
	res++;
      }
    }
    if( i == 0 )
      i += (MEM_ROWS + MEM_BANKS_LOG);
    else
      i++;
  }
  
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
  return(res);
}

/*
  Visual LED test -- requires operator intervention to witness if the LEDs flash
 */
int test_leds(void) {
  int res = 0;
#ifdef CSR_LOOPTEST_BASE
  int last_event;
  int i;
  
  printf( "  {\n");
  printf( "    \"subtest_name\":\"LED\", \n" );
  printf( "    \"failures\":[\n" );
  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/8);

  for( i = 0; i < 21; i++ ) {
    while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4) )
      ;
    looptest_leds_write(1 << (i % 3));  
  }
  looptest_leds_write(0);  

  // can't really generate an error code, it's a visual test...so this is more of a "unclear" rather than "PASS/FAIL"
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
#endif
  return res;
}

/*
  Fan test -- requires operator intervention to witness if the fan stops rotating
 */
int test_fan(void) {
  int res = 0;
#ifdef CSR_LOOPTEST_BASE
  int last_event;
  int i;
  
  printf( "  {\n");
  printf( "    \"subtest_name\":\"fan\", \n" );
  printf( "    \"info\":[      \n      " );

  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY);
  for( i = 0; i < 4; i++ ) {
    looptest_fan_pwm_write(i & 1);
    if( i & 1 ) {
      printf( "{\"state\":\"spinning\"}" );
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
    } else {
      printf( "{\"state\":\"stopped\"}" );
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
    }
    if( i < 3 ) {
      printf( ", " );
    }
  }
  looptest_fan_pwm_write(1);  

  // can't really generate an error code, it's a visual test...so this is more of a "unclear" rather than "PASS/FAIL"
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
#endif
  return res;
}

int test_sdcard(void) {
  int res = 0;
  int firstfail = 1;

#ifdef CSR_SDCORE_BASE
  printf( "  {\n");
  printf( "    \"subtest_name\":\"SD\", \n" );
  printf( "    \"failures\":[\n" );
  // sd clock defaults to 5MHz in this implementation, don't call frequency init...
  res += sdcard_init();
#ifdef FORCERR
  res++;
#endif
  if( res != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"%s\", ", "SD" );
    printf( "     \"syndrome\": {\"init\":%d}}", res );
  }
  printf( "\n    ],\n" );
  
  printf( "    \"info\":[\n" );
  if( res == 0 )
    res += sdcard_test(2);
  printf( "\n    ],\n" );
  
  printf( "    \"subtest_errcount\":%d\n  }", res );
#endif
  return res;
}

// common kernel function for simple loopbacks
int loopback_kernel( void (*tx_func)(unsigned char), int txbit, unsigned char (*rx_func)(void), int rxbit, char *name, int *firstfail ) {
  int res = 0;
  int i;
  unsigned int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};

  for( i = 0; i < 100; i++ ) {
    val = i & 0x1;
    tx_func((unsigned char) (val << txbit));
    if( (rx_func() & (1 << rxbit)) != (val << rxbit) ) {
      syndrome[val] = rx_func();
      mini_ret++;
    }
  }
#ifdef FORCERR
  mini_ret++;
#endif
  if( mini_ret != 0 ) {
    if( *firstfail ) {
      *firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "      {\"name\":\"%s\",\n", name );
    printf( "       \"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}", syndrome[1], syndrome[0], mini_ret );
    res++;
  }
  tx_func(0);
  return res;
}

// second copy of above to handle the "unsigned short" case (reduce compiler warnings)
int loopback_kernel_u16( void (*tx_func)(unsigned short), int txbit, unsigned short (*rx_func)(void), int rxbit, char *name, int *firstfail ) {
  int res = 0;
  int i;
  unsigned int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};

  for( i = 0; i < 100; i++ ) {
    val = i & 0x1;
    tx_func((unsigned short) (val << txbit));
    if( (rx_func() & (1 << rxbit)) != (val << rxbit) ) {
      syndrome[val] = rx_func();
      mini_ret++;
    }
  }
#ifdef FORCERR
  mini_ret++;
#endif
  if( mini_ret != 0 ) {
    if( *firstfail ) {
      *firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "      {\"name\":\"%s\",\n", name );
    printf( "       \"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}", syndrome[1], syndrome[0], mini_ret );
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
  int firstfail = 1;
  
#ifdef CSR_LOOPTEST_BASE
  printf( "  {\n");
  printf( "    \"subtest_name\":\"USB\", \n" );
  printf( "    \"failures\":[\n" );
  res += loopback_kernel( looptest_fusb_tx_write, 0, looptest_fusb_rx_read, 0, "USB", &firstfail );
  
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
#endif
  return res;
}

/*
  Simple loopback test of low-frequency signals -- requires plug-in to PCIe loopback slot, and MCUINT loopback
 */
int test_loopback(void) {
  int res = 0;
  int firstfail = 1;

#ifdef CSR_LOOPTEST_BASE
  printf( "  {\n");
  printf( "    \"subtest_name\":\"loopback\", \n" );
  printf( "    \"failures\":[\n" );

  res += loopback_kernel( looptest_mcu_tx_write, 0, looptest_mcu_rx_read, 0, "MCUINT", &firstfail );
  res += loopback_kernel( looptest_sm_tx_write, 0, looptest_sm_rx_read, 0, "SM", &firstfail );

  // this requires a particular wiring on the PCIe test connector, note order
  res += loopback_kernel_u16( looptest_hax_tx_write, 8, looptest_hax_rx_read, 7, "HAX8->7", &firstfail );
  res += loopback_kernel_u16( looptest_hax_tx_write, 1, looptest_hax_rx_read, 4, "HAX1->4", &firstfail );
  res += loopback_kernel_u16( looptest_hax_tx_write, 9, looptest_hax_rx_read, 3, "HAX9->3", &firstfail );
  res += loopback_kernel_u16( looptest_hax_tx_write, 0, looptest_hax_rx_read, 2, "HAX0->2", &firstfail );
  res += loopback_kernel_u16( looptest_hax_tx_write, 6, looptest_pcie_rx_read, 1, "HAX6->WAKE", &firstfail );
  res += loopback_kernel_u16( looptest_hax_tx_write, 5, looptest_pcie_rx_read, 0, "HAX5->PERST", &firstfail );
  
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
#endif
  return res;
}

/*
  Loopback test of high-speed GTP signals -- requires each GTP link to wire Tx to Rx
 */
#define GTP_ITERS 10000000
int test_gtp(void) {
  int res = 0;
  int diff = 0;
  int firstfail = 1;

#ifdef CSR_GTP0_BASE
  printf( "  {\n");
  printf( "    \"subtest_name\":\"GTP\", \n" );
  printf( "    \"failures\":[\n" );

  int i;
  // accumulate "real time" errors over about 0.5 seconds
  for( i = 0; i < GTP_ITERS; i++ ) {
    res += gtp0_rx_gtp_prbs_err_read();
  }
#ifdef FORCERR
  res++;
#endif
  if( res != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"GTP0\",\n" );
    printf( "     \"syndrome\": {\"errors\":%d, \"low\":%d, \"reps\":%d}}", res );
  }
#endif
  diff = res;

#ifdef CSR_GTP1_BASE
  for( i = 0; i < GTP_ITERS; i++ ) {
    res += gtp1_rx_gtp_prbs_err_read();
  }
#ifdef FORCERR
  res++;
#endif
  if( res - diff != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"GTP1\",\n" );
    printf( "     \"syndrome\": {\"errors\":%d, \"low\":%d, \"reps\":%d}}", res - diff );
  }
#endif

  diff = res;
#ifdef CSR_GTP2_BASE
  for( i = 0; i < GTP_ITERS; i++ ) {
    res += gtp2_rx_gtp_prbs_err_read();
  }
#ifdef FORCERR
  res++;
#endif
  if( res - diff != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"GTP2\",\n" );
    printf( "     \"syndrome\": {\"errors\":%d, \"low\":%d, \"reps\":%d}}", res - diff );
  }
#endif

  diff = res;
#ifdef CSR_GTP3_BASE
  for( i = 0; i < GTP_ITERS; i++ ) {
    res += gtp3_rx_gtp_prbs_err_read();
  }
#ifdef FORCERR
  res++;
#endif
  if( res - diff != 0 ) {
    if( firstfail ) {
      firstfail = 0;
    } else {
      printf( ",\n" );
    }
    printf( "    {\"name\":\"GTP3\",\n" );
    printf( "     \"syndrome\": {\"errors\":%d, \"low\":%d, \"reps\":%d}}", res - diff );
  }
#endif
  
  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
  
  return res;
}

/*
  Check XADC signals -- voltages & temp in range
 */
int test_xadc(void) {
  int res = 0;
  
  double temp;
  double vccint, vccaux, vccbram;
  
  printf( "  {\n");
  printf( "    \"subtest_name\":\"XADC\", \n" );

  temp = ((double)xadc_temperature_read()) * 503.975 / 4096.0 - 273.15;
  vccint = ((double)xadc_vccint_read()) / ((double)0x555);
  vccaux = ((double)xadc_vccaux_read()) / ((double)0x555);
  vccbram = ((double)xadc_vccbram_read()) / ((double)0x555);

  printf( "    \"temp\":%d.%01d, ", (int) temp, (int) (temp - (double)((int)temp)) * 10);
  if( temp < 4.0 || temp > 108.0 ) {  // temperature sensor has ~6C max error, plus give 2 deg margin
    res++;
  }
  printf( "\"vint\":\"%d.%02d\", ", (int) vccint, (int) ((vccint - (double)((int) vccint)) * 100));
  if( vccint < (0.95 * 0.98) || vccint > (1.05 * 1.02) ) { // there's a +/-2% error on XADC readings for supplies
    res++;
  }
  printf( "\"vaux\":\"%d.%02d\", ", (int) vccaux, (int) ((vccaux - (double)((int) vccaux)) * 100) );
  if( vccaux < (1.71 * 0.98) || vccaux > (1.89 * 1.02) ) {
    res++;
  }
  printf( "\"vbram\":\"%d.%02d\", \n", (int) vccbram, (int) ((vccbram - (double)((int) vccbram)) * 100) );
  if( vccbram < (0.95 * 0.98) || vccbram > (1.05 * 1.02) ) {
    res++;
  }

  printf( "    \"failures\":[\n" );
#ifdef FORCERR
  res++;
#endif
  if( res != 0 ) {
    printf( "    {\"name\":\"XADC\"}" );
  }

  printf( "\n    ],\n" );
  printf( "    \"subtest_errcount\":%d\n  }", res );
    
  return res;
  }

int test_board(int test_number) {
  int result = 0;

  printf( "var netv2tester_output = \n" );
  printf( "{\n" );
  printf( "  \"model\":\"NeTV2MVP\",\n" );
  printf( "  \"dna\":\"%016llx\",\n", dna_id_read() );
  printf( "  \"subtests\":[\n" );
    
  if( test_number == XADC_TEST || test_number == ALL_TESTS ) {
    result += test_xadc();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == LOOPBACK_TEST || test_number == ALL_TESTS ) {
    result += test_loopback();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == LED_TEST || test_number == ALL_TESTS ) {
    result += test_leds();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == FAN_TEST || test_number == ALL_TESTS ) {
    result += test_fan();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == USB_TEST || test_number == ALL_TESTS ) {
    result += test_usb();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == GTP_TEST || test_number == ALL_TESTS ) {
    result += test_gtp();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == VIDEO_TEST || test_number == ALL_TESTS ) {
    result += test_video();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == SDCARD_TEST || test_number == ALL_TESTS ) {
    result += test_sdcard();
    if( test_number == ALL_TESTS ) printf( ",\n" );
  }
  if( test_number == MEMORY_TEST || test_number == ALL_TESTS ) {
    result += test_memory();
    // last test has no comma for proper JSON
  }

  printf( "  ],\n" );
  if( test_number == ALL_TESTS )
    printf( "  \"test_errcount\":%d \n}\n", result );
  
  return result;
}
