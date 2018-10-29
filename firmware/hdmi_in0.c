#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <system.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include "flags.h"

#ifdef CSR_HDMI_IN0_BASE

#include "hdmi_in0.h"

#ifdef IDELAYCTRL_CLOCK_FREQUENCY
static int idelay_freq = IDELAYCTRL_CLOCK_FREQUENCY;
#else
static int idelay_freq = 200000000; // default to 200 MHz
#endif

int hdmi_in0_debug = 0;
int hdmi_in0_fb_index;

#define FRAMEBUFFER_COUNT 4
#define FRAMEBUFFER_MASK (FRAMEBUFFER_COUNT - 1)

#define HDMI_IN0_FRAMEBUFFERS_BASE (0x00000000 + 0x100000)
#define HDMI_IN0_FRAMEBUFFERS_SIZE (1920*1080*4)

//#define CLEAN_COMMUTATION
#define DEBUG

#define HDMI_IN0_PHASE_ADJUST_WER_THRESHOLD 1

unsigned int hdmi_in0_framebuffer_base(char n) {
	return HDMI_IN0_FRAMEBUFFERS_BASE + n*HDMI_IN0_FRAMEBUFFERS_SIZE;
}

#ifdef HDMI_IN0_INTERRUPT
static int hdmi_in0_fb_slot_indexes[2];
static int hdmi_in0_next_fb_index;
#endif

static int hdmi_in0_hres, hdmi_in0_vres;

extern void processor_update(void);

static int has_converged = 1;
static int converged_phase[3] = {2,-16,-16};

#ifdef HDMI_IN0_INTERRUPT
void hdmi_in0_isr(void)
{
	int fb_index = -1;
	int length;
	int expected_length;
	unsigned int address_min, address_max;

	printf ("+");
	address_min = HDMI_IN0_FRAMEBUFFERS_BASE & 0x0fffffff;
	address_max = address_min + HDMI_IN0_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT;
	if((hdmi_in0_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING)
		&& ((hdmi_in0_dma_slot0_address_read() < address_min) || (hdmi_in0_dma_slot0_address_read() > address_max)))
		printf("hdmi_in0: slot0: stray DMA\r\n");
	if((hdmi_in0_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING)
		&& ((hdmi_in0_dma_slot1_address_read() < address_min) || (hdmi_in0_dma_slot1_address_read() > address_max)))
		printf("hdmi_in0: slot1: stray DMA\r\n");

#ifdef CLEAN_COMMUTATION
	if((hdmi_in0_resdetection_hres_read() != hdmi_in0_hres)
	  || (hdmi_in0_resdetection_vres_read() != hdmi_in0_vres)) {
		/* Dump frames until we get the expected resolution */
		if(hdmi_in0_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING) {
			hdmi_in0_dma_slot0_address_write(hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[0]));
			hdmi_in0_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
		}
		if(hdmi_in0_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING) {
			hdmi_in0_dma_slot1_address_write(hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[1]));
			hdmi_in0_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
		}
		return;
	}
#endif

	expected_length = hdmi_in0_hres*hdmi_in0_vres*2;
	if(hdmi_in0_dma_slot0_status_read() == DVISAMPLER_SLOT_PENDING) {
		length = hdmi_in0_dma_slot0_address_read() - (hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[0]) & 0x0fffffff);
		if(length == expected_length) {
			fb_index = hdmi_in0_fb_slot_indexes[0];
			hdmi_in0_fb_slot_indexes[0] = hdmi_in0_next_fb_index;
			hdmi_in0_next_fb_index = (hdmi_in0_next_fb_index + 1) & FRAMEBUFFER_MASK;
		} else {
#ifdef DEBUG
			printf("hdmi_in0: slot0: unexpected frame length: %d\r\n", length);
#endif
		}
		hdmi_in0_dma_slot0_address_write(hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[0]));
		hdmi_in0_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
	}
	if(hdmi_in0_dma_slot1_status_read() == DVISAMPLER_SLOT_PENDING) {
		length = hdmi_in0_dma_slot1_address_read() - (hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[1]) & 0x0fffffff);
		if(length == expected_length) {
			fb_index = hdmi_in0_fb_slot_indexes[1];
			hdmi_in0_fb_slot_indexes[1] = hdmi_in0_next_fb_index;
			hdmi_in0_next_fb_index = (hdmi_in0_next_fb_index + 1) & FRAMEBUFFER_MASK;
		} else {
#ifdef DEBUG
			printf("hdmi_in0: slot1: unexpected frame length: %d\r\n", length);
#endif
		}
		hdmi_in0_dma_slot1_address_write(hdmi_in0_framebuffer_base(hdmi_in0_fb_slot_indexes[1]));
		hdmi_in0_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
	}

	if(fb_index != -1)
		hdmi_in0_fb_index = fb_index;
	processor_update();
}
#endif

static int hdmi_in0_connected;
int hdmi_in0_locked;

void hdmi_in0_init_video(int hres, int vres)
{
	hdmi_in0_clocking_mmcm_reset_write(1);
	hdmi_in0_connected = hdmi_in0_locked = 0;
	hdmi_in0_hres = hres; hdmi_in0_vres = vres;

#ifdef  HDMI_IN0_INTERRUPT
	unsigned int mask;

	puts( "setting up HDMI0 interrupts\n" );

	hdmi_in0_dma_frame_size_write(hres*vres*2);
	hdmi_in0_fb_slot_indexes[0] = 0;
	hdmi_in0_dma_slot0_address_write(hdmi_in0_framebuffer_base(0));
	hdmi_in0_dma_slot0_status_write(DVISAMPLER_SLOT_LOADED);
	hdmi_in0_fb_slot_indexes[1] = 1;
	hdmi_in0_dma_slot1_address_write(hdmi_in0_framebuffer_base(1));
	hdmi_in0_dma_slot1_status_write(DVISAMPLER_SLOT_LOADED);
	hdmi_in0_next_fb_index = 2;

	hdmi_in0_dma_ev_pending_write(hdmi_in0_dma_ev_pending_read());
	hdmi_in0_dma_ev_enable_write(0x3);
	mask = irq_getmask();
	mask |= 1 << HDMI_IN0_INTERRUPT;
	irq_setmask(mask);

	hdmi_in0_fb_index = 3;
	
#endif

#ifdef CSR_HDMI_IN0_DATA0_CAP_EYE_BIT_TIME_ADDR
	int bit_time = 18;  // 18 if you should round up, not truncate
	printf( "hdmi_in0: setting algo 2 eye time to %d IDELAY periods\n", bit_time );
	hdmi_in0_data0_cap_eye_bit_time_write(bit_time);
	hdmi_in0_data1_cap_eye_bit_time_write(bit_time);
	hdmi_in0_data2_cap_eye_bit_time_write(bit_time);

	hdmi_in0_data0_cap_algorithm_write(2); // 1 is just delay criteria change, 2 is auto-delay machine
	hdmi_in0_data1_cap_algorithm_write(2);
	hdmi_in0_data2_cap_algorithm_write(2);
	hdmi_in0_algorithm = 2;
	hdmi_in0_data0_cap_auto_ctl_write(7);
	hdmi_in0_data1_cap_auto_ctl_write(7);
	hdmi_in0_data2_cap_auto_ctl_write(7);
#endif
	
}

void hdmi_in0_disable(void)
{
#ifdef HDMI_IN0_INTERRUPT
	unsigned int mask;

	mask = irq_getmask();
	mask &= ~(1 << HDMI_IN0_INTERRUPT);
	irq_setmask(mask);

	hdmi_in0_dma_slot0_status_write(DVISAMPLER_SLOT_EMPTY);
	hdmi_in0_dma_slot1_status_write(DVISAMPLER_SLOT_EMPTY);
#endif
	hdmi_in0_clocking_mmcm_reset_write(1);
}

void hdmi_in0_clear_framebuffers(void)
{
	int i;
	flush_l2_cache();
	volatile unsigned int *framebuffer = (unsigned int *)(MAIN_RAM_BASE + HDMI_IN0_FRAMEBUFFERS_BASE);
	for(i=0; i<(HDMI_IN0_FRAMEBUFFERS_SIZE*FRAMEBUFFER_COUNT)/4; i++) {
		framebuffer[i] = 0x80108010; /* black in YCbCr 4:2:2*/
	}
}

int hdmi_in0_d0, hdmi_in0_d1, hdmi_in0_d2;
int hdmi_in0_algorithm = 0;
 
void hdmi_in0_print_status(void)
{
	hdmi_in0_data0_wer_update_write(1);
	hdmi_in0_data1_wer_update_write(1);
	hdmi_in0_data2_wer_update_write(1);
	printf("hdmi_in0: ph:%4d(%2d/%2d)%02x %4d(%2d/%2d)%02x %4d(%2d/%2d)%02x // charsync:%d%d%d [%d %d %d] // WER:%3d %3d %3d // chansync:%d // res:%dx%d\r\n",
	       hdmi_in0_d0, hdmi_in0_data0_cap_cntvalueout_m_read(), hdmi_in0_data0_cap_cntvalueout_s_read(), hdmi_in0_data0_cap_lateness_read(),
	       hdmi_in0_d1, hdmi_in0_data1_cap_cntvalueout_m_read(), hdmi_in0_data1_cap_cntvalueout_s_read(), hdmi_in0_data1_cap_lateness_read(),
	       hdmi_in0_d2, hdmi_in0_data2_cap_cntvalueout_m_read(), hdmi_in0_data2_cap_cntvalueout_s_read(), hdmi_in0_data2_cap_lateness_read(),
	        
		hdmi_in0_data0_charsync_char_synced_read(),
		hdmi_in0_data1_charsync_char_synced_read(),
		hdmi_in0_data2_charsync_char_synced_read(),
		hdmi_in0_data0_charsync_ctl_pos_read(),
		hdmi_in0_data1_charsync_ctl_pos_read(),
		hdmi_in0_data2_charsync_ctl_pos_read(),
		hdmi_in0_data0_wer_value_read(),
		hdmi_in0_data1_wer_value_read(),
		hdmi_in0_data2_wer_value_read(),
		hdmi_in0_chansync_channels_synced_read(),
		hdmi_in0_resdetection_hres_read(),
		hdmi_in0_resdetection_vres_read());
}

static int hdmi_in0_eye[3];
static int hdmi_in0_phase_target;

void hdmi_in0_update_eye() {
  // increasing the delta between master and slave pushes the master farther from the transition point
  hdmi_in0_eye[0] = hdmi_in0_data0_cap_cntvalueout_s_read() - hdmi_in0_data0_cap_cntvalueout_m_read();
  hdmi_in0_eye[1] = hdmi_in0_data1_cap_cntvalueout_s_read() - hdmi_in0_data1_cap_cntvalueout_m_read();
  hdmi_in0_eye[2] = hdmi_in0_data2_cap_cntvalueout_s_read() - hdmi_in0_data2_cap_cntvalueout_m_read();
}

int hdmi_in0_calibrate_delays(int freq)
{
	int i, phase_detector_delay;

	int iodelay_tap_duration;

	if( hdmi_in0_algorithm == 0 ) {
	if( idelay_freq == 400000000 ) {
	  iodelay_tap_duration = 39;
	} else {
	  iodelay_tap_duration = 78;
	}
	
	hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_RST);
	hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_RST);
	hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_RST);
	hdmi_in0_data0_cap_phase_reset_write(1);
	hdmi_in0_data1_cap_phase_reset_write(1);
	hdmi_in0_data2_cap_phase_reset_write(1);
	hdmi_in0_d0 = hdmi_in0_d1 = hdmi_in0_d2 = 0;

	/* preload slave phase detector idelay with 90° phase shift
	  (78 ps taps on 7-series) */
	printf( "idelay_freq: %d\n", idelay_freq );
	// TODO: VALIDATE /4 SETTING
	phase_detector_delay = 10000000/(4*freq*iodelay_tap_duration) + 1; // +1 because we should round not truncate
	hdmi_in0_phase_target = phase_detector_delay;
	printf("HDMI in0 calibrate delays @ %dMHz, %d taps\n", freq, phase_detector_delay);
	for(i=0; i<phase_detector_delay; i++) {
		hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
		hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
		hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
	}
	hdmi_in0_update_eye();
        }
	return 1;
}

void hdmi_in0_nudge_eye(int chan, int amount) {
  int i;
  if( chan == 0 ) {
    if( amount > 0 ) {
      for( i = 0; i < amount; i++ ) {
	hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
      }
    } else if( amount < 0 ) {
      for( i = 0; i < -amount; i++ ) {
	hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_DEC);
      }
    }
  }
  if( chan == 1 ) {
    if( amount > 0 ) {
      for( i = 0; i < amount; i++ ) {
	hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
      }
    } else if( amount < 0 ) {
      for( i = 0; i < -amount; i++ ) {
	hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_DEC);
      }
    }
  }
  if( chan == 2 ) {
    if( amount > 0 ) {
      for( i = 0; i < amount; i++ ) {
	hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_INC);
      }
    } else if( amount < 0 ) {
      for( i = 0; i < -amount; i++ ) {
	hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_SLAVE_DEC);
      }
    }
  }

  hdmi_in0_update_eye();
  printf( "hdmi in0 eye nudged: %d %d %d\n", hdmi_in0_eye[0], hdmi_in0_eye[1], hdmi_in0_eye[2] );
}

// it doesn't make sense to let the delay controller wrap around
// you're trying to control the distance between master and slave,
// and 31 taps * 39ps = 2356 ps, but the bit period is 673ps @ 1080p
// so that's 3.5 bit periods per delay sweep; when the delay wraps around
// to zero on the slave, you end up trying to align to data that's several
// cycles old
#define WRAP_LIMIT 17
void hdmi_in0_fixup_eye() {
  int wrap_amount;
  int i;
  int delay;

  delay = hdmi_in0_data0_cap_cntvalueout_m_read();
  if( (delay > WRAP_LIMIT) && (delay != 31) ) {
    for (i=0; i < delay; i++) {
      hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d0--;
    }
  } else if( delay == 31 ) {
    for(i=0; i < (WRAP_LIMIT); i++ ) {
      hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d0++;
    }
  }

  delay = hdmi_in0_data1_cap_cntvalueout_m_read();
  if( (delay > WRAP_LIMIT) && (delay != 31) ) {
    for (i=0; i < delay; i++) {
      hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d1--;
    }
  } else if( delay == 31 ) {
    for(i=0; i < (WRAP_LIMIT); i++ ) {
      hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d1++;
    }
  }

  delay = hdmi_in0_data2_cap_cntvalueout_m_read();
  if( (delay > WRAP_LIMIT) && (delay != 31) ) {
    for (i=0; i < delay; i++) {
      hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d2--;
    }
  } else if( delay == 31 ) {
    for(i=0; i < (WRAP_LIMIT); i++ ) {
      hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d2++;
    }
  }
  
}
    

int hdmi_in0_adjust_phase(void)
{
	switch(hdmi_in0_data0_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				                             DVISAMPLER_DELAY_SLAVE_DEC);
			hdmi_in0_d0--;
			hdmi_in0_data0_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				                             DVISAMPLER_DELAY_SLAVE_INC);
			hdmi_in0_d0++;
			hdmi_in0_data0_cap_phase_reset_write(1);
			break;
	}
	switch(hdmi_in0_data1_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				                             DVISAMPLER_DELAY_SLAVE_DEC);
			hdmi_in0_d1--;
			hdmi_in0_data1_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				                             DVISAMPLER_DELAY_SLAVE_INC);
			hdmi_in0_d1++;
			hdmi_in0_data1_cap_phase_reset_write(1);
			break;
	}
	switch(hdmi_in0_data2_cap_phase_read()) {
		case DVISAMPLER_TOO_LATE:
			hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				                             DVISAMPLER_DELAY_SLAVE_DEC);
			hdmi_in0_d2--;
			hdmi_in0_data2_cap_phase_reset_write(1);
			break;
		case DVISAMPLER_TOO_EARLY:
			hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				                             DVISAMPLER_DELAY_SLAVE_INC);
			hdmi_in0_d2++;
			hdmi_in0_data2_cap_phase_reset_write(1);
			break;
	}

	hdmi_in0_fixup_eye();
	hdmi_in0_update_eye();
	
	return 1;
}

void hdmi_in0_set_phase(int *converged_phase);

void hdmi_in0_set_phase(int *converged_phase) {
  int delta;
  int i;

  delta = hdmi_in0_d0 + converged_phase[0];
  if( delta < 0 ) {
    for( i = 0; i < -delta; i++ ) {
      hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d0--;
      hdmi_in0_data0_cap_phase_reset_write(1);
    }
  } else if( delta > 0 ) {
    for( i = 0; i < delta; i++ ) {
      hdmi_in0_data0_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d0++;
      hdmi_in0_data0_cap_phase_reset_write(1);
    }
  }

  delta = hdmi_in0_d1 + converged_phase[1];
  if( delta < 0 ) {
    for( i = 0; i < -delta; i++ ) {
      hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d1--;
      hdmi_in0_data1_cap_phase_reset_write(1);
    }
  } else if( delta > 0 ) {
    for( i = 0; i < delta; i++ ) {
      hdmi_in0_data1_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d1++;
      hdmi_in0_data1_cap_phase_reset_write(1);
    }
  }
  
  delta = hdmi_in0_d2 + converged_phase[1];
  if( delta < 0 ) {
    for( i = 0; i < -delta; i++ ) {
      hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_DEC |
				       DVISAMPLER_DELAY_SLAVE_DEC);
      hdmi_in0_d2--;
      hdmi_in0_data2_cap_phase_reset_write(1);
    }
  } else if( delta > 0 ) {
    for( i = 0; i < delta; i++ ) {
      hdmi_in0_data2_cap_dly_ctl_write(DVISAMPLER_DELAY_MASTER_INC |
				       DVISAMPLER_DELAY_SLAVE_INC);
      hdmi_in0_d2++;
      hdmi_in0_data2_cap_phase_reset_write(1);
    }
  }
  
}

// new compiler, vexriscv runs too fast for phase init to work well
// put some delay in the loop...
static void phase_delay(void) {
  volatile int i;
  for( i = 0; i < 1000; i++ )
    ;
}

int hdmi_in0_init_phase(void)
{
	int o_d0, o_d1, o_d2;
	int i, j;

        if( hdmi_in0_algorithm == 0 ) { 
	if( has_converged ) {
	  // use the last convergence as the starting point guess for initialization
	  printf( "using last converged phase as starting point for phase init\n" );
	  hdmi_in0_set_phase(converged_phase);
	}

	for(i=0;i<100;i++) {
		o_d0 = hdmi_in0_d0;
		o_d1 = hdmi_in0_d1;
		o_d2 = hdmi_in0_d2;
		for(j=0;j<1000;j++) {
		  if(!hdmi_in0_adjust_phase())
		    return 0;
		  phase_delay();
		}
		if((abs(hdmi_in0_d0 - o_d0) < 4) && (abs(hdmi_in0_d1 - o_d1) < 4) && (abs(hdmi_in0_d2 - o_d2) < 4))
			return 1;
	}
	}
	return 0;
}

int hdmi_in0_phase_startup(int freq)
{
	int ret;
	int attempts;

	attempts = 0;
	if( hdmi_in0_algorithm == 0 ) {
	while(1) {
		attempts++;
		hdmi_in0_calibrate_delays(freq);
		if(hdmi_in0_debug)
			printf("hdmi_in0: delays calibrated\r\n");
		ret = hdmi_in0_init_phase();
		if(ret) {
			if(hdmi_in0_debug)
				printf("hdmi_in0: phase init OK\r\n");
			return 1;
		} else {
			printf("hdmi_in0: phase init failed\r\n");
			if(attempts > 3) {
				printf("hdmi_in0: giving up\r\n");
				hdmi_in0_calibrate_delays(freq);
				return 0;
			}
		}
	}
	}
}

static void hdmi_in0_check_overflow(void)
{
#ifdef HDMI_IN0_INTERRUPT
	if(hdmi_in0_frame_overflow_read()) {
		printf("hdmi_in0: FIFO overflow\r\n");
		hdmi_in0_frame_overflow_write(1);
	}
#endif
}

static int hdmi_in0_clocking_locked_filtered(void)
{
	static int lock_start_time;
	static int lock_status;

	if(hdmi_in0_clocking_locked_read()) {
		switch(lock_status) {
			case 0:
				elapsed(&lock_start_time, -1);
				lock_status = 1;
				break;
			case 1:
				if(elapsed(&lock_start_time, SYSTEM_CLOCK_FREQUENCY/8))
					lock_status = 2;
				break;
			case 2:
				return 1;
		}
	} else
		lock_status = 0;
	return 0;
}

static int hdmi_in0_get_wer(void){
	int wer = 0;
	wer += hdmi_in0_data0_wer_value_read();
	wer += hdmi_in0_data1_wer_value_read();
	wer += hdmi_in0_data2_wer_value_read();
	return wer;
} 

void hdmi_in0_service(int freq)
{
	static int last_event;

	if(hdmi_in0_connected) {
		if(!hdmi_in0_edid_hpd_notif_read()) {
			if(hdmi_in0_debug)
				printf("hdmi_in0: disconnected\r\n");
			hdmi_in0_connected = 0;
			hdmi_in0_locked = 0;
			hdmi_in0_clocking_mmcm_reset_write(1);
			hdmi_in0_clear_framebuffers();
		} else {
			if(hdmi_in0_locked) {
				if(hdmi_in0_clocking_locked_filtered()) {
					if(elapsed(&last_event, SYSTEM_CLOCK_FREQUENCY/16)) {
					  hdmi_in0_data0_wer_update_write(1);
					  hdmi_in0_data1_wer_update_write(1);
					  hdmi_in0_data2_wer_update_write(1);
					  if(hdmi_in0_debug)
					    hdmi_in0_print_status();
					  if(hdmi_in0_get_wer() >= HDMI_IN0_PHASE_ADJUST_WER_THRESHOLD) {
					    if( hdmi_in0_algorithm == 0 )
					  	  hdmi_in0_adjust_phase();
					  } else {
					    has_converged = 1;
					    converged_phase[0] = hdmi_in0_d0;
					    converged_phase[1] = hdmi_in0_d1;
					    converged_phase[2] = hdmi_in0_d2;
					  }
					}
				} else {
					if(hdmi_in0_debug)
						printf("hdmi_in0: lost PLL lock\r\n");
					hdmi_in0_locked = 0;
					hdmi_in0_clear_framebuffers();
				}
			} else {
				if(hdmi_in0_clocking_locked_filtered()) {
					if(hdmi_in0_debug)
						printf("hdmi_in0: PLL locked\r\n");
					hdmi_in0_phase_startup(freq);
					if(hdmi_in0_debug)
						hdmi_in0_print_status();
					hdmi_in0_locked = 1;
				}
			}
		}
	} else {
		if(hdmi_in0_edid_hpd_notif_read()) {
			if(hdmi_in0_debug)
				printf("hdmi_in0: connected\r\n");
			hdmi_in0_connected = 1;
			hdmi_in0_clocking_mmcm_reset_write(0);
		}
	}
	hdmi_in0_check_overflow();
}

#endif
