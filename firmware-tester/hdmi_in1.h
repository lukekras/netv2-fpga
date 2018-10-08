#include <stdbool.h>
#include "framebuffer.h"

#ifndef __HDMI_IN1_H
#define __HDMI_IN1_H

#ifdef HDMI_IN1_INDEX
#error "HDMI_IN1_INDEX already defined!"
#endif

#define HDMI_IN1_INDEX 			1
#define HDMI_IN1_FRAMEBUFFERS_BASE 	FRAMEBUFFER_BASE_HDMI_INPUT(HDMI_IN1_INDEX)

extern int hdmi_in1_debug;
extern int hdmi_in1_fb_index;

fb_ptrdiff_t hdmi_in1_framebuffer_base(char n);

void hdmi_in1_isr(void);
void hdmi_in1_init_video(int hres, int vres);
void hdmi_in1_enable(void);
bool hdmi_in1_status(void);
void hdmi_in1_disable(void);
void hdmi_in1_clear_framebuffers(void);
void hdmi_in1_print_status(void);
int hdmi_in1_calibrate_delays(int freq);
int hdmi_in1_adjust_phase(void);
int hdmi_in1_init_phase(void);
int hdmi_in1_phase_startup(int freq);
void hdmi_in1_service(int freq);
void hdmi_in1_nudge_eye(int chan, int amount);

#endif
