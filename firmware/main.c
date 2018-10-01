#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <irq.h>
#include <uart.h>
#include <time.h>
#include <generated/csr.h>
#include <generated/mem.h>
#include "flags.h"
#include <console.h>
#include <system.h>

#include "config.h"
#include "ci.h"
#include "processor.h"
#include "pattern.h"

void * __stack_chk_guard = (void *) (0xDEADBEEF);
void __stack_chk_fail(void) {
  printf( "stack fail\n" );
}

int main(void)
{
	hdcp_hpd_ena_write(1);  // de-assert hot plug detect while booting
	irq_setmask(0);
	irq_setie(1);
	uart_init();

	//	hdmi_out0_i2c_init();

	puts("\nNeTV2 software built "__DATE__" "__TIME__);

	config_init();
	time_init();

	processor_init();
	processor_update();
	processor_start(config_get(CONFIG_KEY_RESOLUTION));

	ci_prompt();

	hdcp_hpd_ena_write(0); // release hot plug detect once we're into the main loop
	
	while(1) {
	  processor_service();
	  ci_service();
	}

	return 0;
}
