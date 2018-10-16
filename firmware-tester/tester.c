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
#include "uptime.h"

// output validated with https://jsonformatter.curiousconcept.com/
//#define FORCERR   // uncomment to help debug JSON formatting

static long long my_dna = 0LL;
static char model_str[] = "NeTV2MVP";
static char rev_str[] = "PVT1C";
static char tester_ver[] = "1";

void checklenf( int checklen ) {
  if(checklen >= MSGLEN-1)
    printf( "\n{\"internal_error\":\"string overflow\"}\n" );
}

void printj(char *subtest, char *msg) {
  uptime_service();  
  printf( "{ \"model\":\"%s\", \"rev\":\"%s\", \"tester_rev\":\"%s\", \"dna\":\"%lld\", \"time\":%d, \"subtest\":\"%s\", \"msg\": [%s] }\n",
	  model_str, rev_str, tester_ver, my_dna, uptime(), subtest, msg );
}

void errcnt_macro(char *testname, int res) {
  char msg[MSGLEN];
  int checklen;
  
  checklen = snprintf( msg, MSGLEN, "{\"errcnt\":%d}", res );
  checklenf( checklen );
  printj( testname, msg );
}

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
  
#ifdef CSR_HDMI_IN0_BASE  
  char msg[MSGLEN];
  int checklen;
  char testname[] = "video";

  /////////// PLL TEST
  resdiff = result;
  if( hdmi_in0_resdetection_hres_read() != VIDEO_HACTIVE )
    result++;
  if( hdmi_in0_resdetection_vres_read() != VIDEO_VACTIVE )
    result++;
  if( hdmi_in0_freq_value_read() != VIDEO_FREQ )
    result++;
  if( resdiff != result ) {
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": \
{\"hdmi_in0_hres\":%d, \"hdmi_in0_vres\":%d, \"hdmi_in0_freq\":%d}}]}",
			 "HDMI in0 res/freq",
			 hdmi_in0_resdetection_hres_read(), hdmi_in0_resdetection_vres_read(), hdmi_in0_freq_value_read() );
    checklenf( checklen );
    printj( testname, msg );
    
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": \
{\"hdmi_in1_hres\":%d, \"hdmi_in1_vres\":%d, \"hdmi_in1_freq\":%d}}]}",
			 "HDMI in1 res/freq",
			 hdmi_in1_resdetection_hres_read(), hdmi_in1_resdetection_vres_read(), hdmi_in1_freq_value_read() );
    checklenf( checklen );
    printj( testname, msg );
    
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
	checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": \
{\"hdmi_in0\":\"mismatch\", \"offset\":%d, \"got\":%d, \"expected\":%d}}]}",
			     "HDMI in0 link",
			     i, expected, framebuffer[i] );
	checklenf( checklen );
	printj( testname, msg );

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
	checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": \
{\"hdmi_in1\":\"mismatch\", \"offset\":%d, \"got\":%d, \"expected\":%d}}]}",
			     "HDMI in1 link",
			     i, expected, framebuffer[i] );
	checklenf( checklen );
	printj( testname, msg );

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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"hdmi_in0 DDC\"}]}" );
    checklenf( checklen );
    printj( testname, msg );
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"hdmi_in1 DDC\"}]}" );
    checklenf( checklen );
    printj( testname, msg );
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}]}",
			 "CEC", syndrome[1], syndrome[0], mini_ret );
    checklenf( checklen );
    printj( testname, msg );
    
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"HPD force unplug\"}]}" );
    checklenf( checklen );
    printj( testname, msg );
  }
  looptest_rx_forceunplug_write(0);

  errcnt_macro( testname, result );
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
  
  char msg[MSGLEN];
  int checklen;
  char testname[] = "RAM";

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
	  checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"syndrome\":{\"addr\":%d, \"got\":%d, \"expected\":%d}}]}",
			       &(mem[(1 << i) + j]), mem[(1 << i) + j], val );
	  checklenf( checklen );
	  printj( testname, msg );
	}
	res++;
      }
    }
    if( i == 0 )
      i += (MEM_ROWS + MEM_BANKS_LOG);
    else
      i++;
  }
  
  errcnt_macro( testname, res );
  return(res);
}

/*
  Visual LED test -- requires operator intervention to witness if the LEDs flash
 */
int test_leds(void) {
  int res = 0;
  
#ifdef CSR_LOOPTEST_BASE
  char msg[MSGLEN];
  int checklen;
  char testname[] = "LED";

  int last_event;
  int i;
  
  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/8);

  for( i = 0; i < 21; i++ ) {
    while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/4) )
      ;
    looptest_leds_write(1 << (i % 3));  
    checklen = snprintf( msg, MSGLEN, "{\"info\":{\"state\":\"pattern %d\"}}", 1 << (i % 3) );
  }
  looptest_leds_write(0);  

  errcnt_macro( testname, res );
#endif
  return res;
}

/*
  Fan test -- requires operator intervention to witness if the fan stops rotating
 */
int test_fan(void) {
  int res = 0;
  
#ifdef CSR_LOOPTEST_BASE
  char msg[MSGLEN];
  int checklen;
  char testname[] = "fan";
  
  int last_event;
  int i;
  
  elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY);
  for( i = 0; i < 4; i++ ) {
    looptest_fan_pwm_write(i & 1);
    if( i & 1 ) {
      checklen = snprintf( msg, MSGLEN, "{\"info\":{\"state\":\"spinning\"}}" );
      checklenf( checklen );
      printj( testname, msg );
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
    } else {
      checklen = snprintf( msg, MSGLEN, "{\"info\":{\"state\":\"stopped\"}}" );
      checklenf( checklen );
      printj( testname, msg );
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
      while( !elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY) )
	;
    }
  }
  looptest_fan_pwm_write(1);  

  errcnt_macro( testname, res );
#endif
  return res;
}

int test_sdcard(void) {
  int res = 0;

#ifdef CSR_SDCORE_BASE

  // sd clock defaults to 5MHz in this implementation, don't call frequency init...
  char msg[MSGLEN];
  int checklen;
  char testname[] = "SD";
  
  res += sdcard_init();
#ifdef FORCERR
  res++;
#endif
  if( res != 0 ) {
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"init\":\"%d\"}]}", res);
    checklenf( checklen );
    printj( testname, msg );
  }
  
  if( res == 0 )
    res += sdcard_test(2);

  errcnt_macro( testname, res );
#endif
  return res;
}

// common kernel function for simple loopbacks
int loopback_kernel( void (*tx_func)(unsigned char), int txbit, unsigned char (*rx_func)(void), int rxbit, char *name, char *testname ) {
  int res = 0;
  int i;
  unsigned int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};
  int checklen;
  char msg[MSGLEN];

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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}]}",
			 name, syndrome[1], syndrome[0], mini_ret );
    checklenf( checklen );
    printj( testname, msg );
    res++;
  }
  tx_func(0);
  return res;
}

// second copy of above to handle the "unsigned short" case (reduce compiler warnings)
int loopback_kernel_u16( void (*tx_func)(unsigned short), int txbit, unsigned short (*rx_func)(void), int rxbit, char *name, char *testname ) {
  int res = 0;
  int i;
  unsigned int val;
  int mini_ret = 0;
  int syndrome[2] = {0, 0};
  int checklen;
  char msg[MSGLEN];

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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"%s\"},{\"syndrome\": {\"high\":%d, \"low\":%d, \"reps\":%d}}]}",
			 name, syndrome[1], syndrome[0], mini_ret );
    checklenf( checklen );
    printj( testname, msg );
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
  
#ifdef CSR_LOOPTEST_BASE
  char testname[] = "USB";
  
  res += loopback_kernel( looptest_fusb_tx_write, 0, looptest_fusb_rx_read, 0, "USB", testname );
  
  errcnt_macro( testname, res );
#endif
  return res;
}

/*
  Simple loopback test of low-frequency signals -- requires plug-in to PCIe loopback slot, and MCUINT loopback
 */
int test_loopback(void) {
  int res = 0;

#ifdef CSR_LOOPTEST_BASE
  char testname[] = "loopback";

  res += loopback_kernel( looptest_mcu_tx_write, 0, looptest_mcu_rx_read, 0, "MCUINT", testname );
  res += loopback_kernel( looptest_sm_tx_write, 0, looptest_sm_rx_read, 0, "SM", testname );

  // this requires a particular wiring on the PCIe test connector, note order
  res += loopback_kernel_u16( looptest_hax_tx_write, 8, looptest_hax_rx_read, 7, "HAX8->7", testname );
  res += loopback_kernel_u16( looptest_hax_tx_write, 1, looptest_hax_rx_read, 4, "HAX1->4", testname );
  res += loopback_kernel_u16( looptest_hax_tx_write, 9, looptest_hax_rx_read, 3, "HAX9->3", testname );
  res += loopback_kernel_u16( looptest_hax_tx_write, 0, looptest_hax_rx_read, 2, "HAX0->2", testname );
  res += loopback_kernel_u16( looptest_hax_tx_write, 6, looptest_pcie_rx_read, 1, "HAX6->WAKE", testname );
  res += loopback_kernel_u16( looptest_hax_tx_write, 5, looptest_pcie_rx_read, 0, "HAX5->PERST", testname );
  
  errcnt_macro( testname, res );
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
  int i;
  char msg[MSGLEN];
  int checklen;
  char testname[] = "GTP";

  diff = res;
#ifdef CSR_GTP0_BASE
  // accumulate "real time" errors over about 0.5 seconds
  for( i = 0; i < GTP_ITERS; i++ ) {
    res += gtp0_rx_gtp_prbs_err_read();
  }
#ifdef FORCERR
  res++;
#endif
  if( res - diff != 0 ) {
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"GTP0\"},{\"errcnt\":%d}]}", res - diff );
    checklenf( checklen );
    printj( testname, msg );
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"GTP1\"},{\"errcnt\":%d}]}", res - diff );
    checklenf( checklen );
    printj( testname, msg );
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"GTP1\"},{\"errcnt\":%d}]}", res - diff );
    checklenf( checklen );
    printj( testname, msg );
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
    checklen = snprintf( msg, MSGLEN, "{\"error\":[{\"name\":\"GTP1\"},{\"errcnt\":%d}]}", res - diff );
    checklenf( checklen );
    printj( testname, msg );
  }
#endif
  
  errcnt_macro( testname, res );  
  return res;
}

/*
  Check XADC signals -- voltages & temp in range
 */
int test_xadc(void) {
  int res = 0;
  
  double temp;
  double vccint, vccaux, vccbram;
  char msg[MSGLEN];
  int checklen;
  char testname[] = "xadc";
  
  temp = ((double)xadc_temperature_read()) * 503.975 / 4096.0 - 273.15;
  vccint = ((double)xadc_vccint_read()) / ((double)0x555);
  vccaux = ((double)xadc_vccaux_read()) / ((double)0x555);
  vccbram = ((double)xadc_vccbram_read()) / ((double)0x555);

  if( temp < 4.0 || temp > 108.0 ) {  // temperature sensor has ~6C max error, plus give 2 deg margin
    res++;
  }
  if( vccint < (0.95 * 0.98) || vccint > (1.05 * 1.02) ) { // there's a +/-2% error on XADC readings for supplies
    res++;
  }
  if( vccaux < (1.71 * 0.98) || vccaux > (1.89 * 1.02) ) {
    res++;
  }
  if( vccbram < (0.95 * 0.98) || vccbram > (1.05 * 1.02) ) {
    res++;
  }
#ifdef FORCERR
  res++;
#endif
  
  checklen = snprintf( msg, MSGLEN, "{\"info\": [\
{\"temp\":%d.%01d}, \
{\"vint\":\"%d.%02d\"}, \
{\"vaux\":\"%d.%02d\"}, \
{\"vbram\":\"%d.%02d\"}]}", (int) temp, (int) (temp - (double)((int)temp)) * 10,
		       (int) vccint, (int) ((vccint - (double)((int) vccint)) * 100),
		       (int) vccaux, (int) ((vccaux - (double)((int) vccaux)) * 100),
		       (int) vccbram, (int) ((vccbram - (double)((int) vccbram)) * 100),
		       res );
  checklenf( checklen );
  printj( testname, msg );

  errcnt_macro( testname, res );  
  return res;
}
 
int test_board(int test_number) {
  int result = 0;

  my_dna = dna_id_read();
  uptime_service();  
  printf( "{ \"model\":\"%s\", \"rev\":\"%s\", \"tester_rev\":\"%s\", \"dna\":\"%lld\", \"start_time\":%d }\n",
	  model_str, rev_str, tester_ver, my_dna, uptime() );

  if( test_number == XADC_TEST || test_number == ALL_TESTS ) {
    result += test_xadc();
  }
  if( test_number == LOOPBACK_TEST || test_number == ALL_TESTS ) {
    result += test_loopback();
  }
  if( test_number == LED_TEST || test_number == ALL_TESTS ) {
    result += test_leds();
  }
  if( test_number == FAN_TEST || test_number == ALL_TESTS ) {
    result += test_fan();
  }
  if( test_number == USB_TEST || test_number == ALL_TESTS ) {
    result += test_usb();
  }
  if( test_number == GTP_TEST || test_number == ALL_TESTS ) {
    result += test_gtp();
  }
  if( test_number == VIDEO_TEST || test_number == ALL_TESTS ) {
    result += test_video();
  }
  if( test_number == SDCARD_TEST || test_number == ALL_TESTS ) {
    result += test_sdcard();
  }
  if( test_number == MEMORY_TEST || test_number == ALL_TESTS ) {
    result += test_memory();
  }

  uptime_service();  
  printf( "{ \"model\":\"%s\", \"rev\":\"%s\", \"tester_rev\":\"%s\", \"dna\":\"%lld\", \"end_time\":%d, \"test_errcount\":%d }\n",
	  model_str, rev_str, tester_ver, my_dna, uptime(), result );
  return result;
}
