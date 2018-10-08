#include <stdio.h>
#include <generated/csr.h>

#include "mmcm.h"
#include "stdio_wrap.h"

/*
 * Despite varying pixel clocks, we must keep the PLL VCO operating
 * in the specified range of 600MHz - 1200MHz.
 */
#ifdef CSR_HDMI_OUT0_DRIVER_CLOCKING_MMCM_RESET_ADDR
void hdmi_out0_driver_clocking_mmcm_write(int adr, int data)
{
	hdmi_out0_driver_clocking_mmcm_adr_write(adr);
	hdmi_out0_driver_clocking_mmcm_dat_w_write(data);
	hdmi_out0_driver_clocking_mmcm_write_write(1);
	while(!hdmi_out0_driver_clocking_mmcm_drdy_read());
}

int hdmi_out0_driver_clocking_mmcm_read(int adr)
{
	hdmi_out0_driver_clocking_mmcm_adr_write(adr);
	hdmi_out0_driver_clocking_mmcm_read_write(1);
	while(!hdmi_out0_driver_clocking_mmcm_drdy_read());
	return hdmi_out0_driver_clocking_mmcm_dat_r_read();
}

MMCM hdmi_out0_driver_clocking_mmcm = {
	.write = &hdmi_out0_driver_clocking_mmcm_write,
	.read = &hdmi_out0_driver_clocking_mmcm_read,
};
#endif

#ifdef CSR_HDMI_OUT1_DRIVER_CLOCKING_MMCM_RESET_ADDR
void hdmi_out1_driver_clocking_mmcm_write(int adr, int data)
{
	hdmi_out1_driver_clocking_mmcm_adr_write(adr);
	hdmi_out1_driver_clocking_mmcm_dat_w_write(data);
	hdmi_out1_driver_clocking_mmcm_write_write(1);
	while(!hdmi_out1_driver_clocking_mmcm_drdy_read());
}

int hdmi_out1_driver_clocking_mmcm_read(int adr)
{
	hdmi_out1_driver_clocking_mmcm_adr_write(adr);
	hdmi_out1_driver_clocking_mmcm_read_write(1);
	while(!hdmi_out1_driver_clocking_mmcm_drdy_read());
	return hdmi_out1_driver_clocking_mmcm_dat_r_read();
}

MMCM hdmi_out1_driver_clocking_mmcm = {
	.write = &hdmi_out1_driver_clocking_mmcm_write,
	.read = &hdmi_out1_driver_clocking_mmcm_read,
};
#endif

#ifdef CSR_HDMI_IN0_CLOCKING_MMCM_RESET_ADDR
void hdmi_in0_clocking_mmcm_write(int adr, int data)
{
	hdmi_in0_clocking_mmcm_adr_write(adr);
	hdmi_in0_clocking_mmcm_dat_w_write(data);
	hdmi_in0_clocking_mmcm_write_write(1);
	while(!hdmi_in0_clocking_mmcm_drdy_read());
}

int hdmi_in0_clocking_mmcm_read(int adr)
{
	hdmi_in0_clocking_mmcm_adr_write(adr);
	hdmi_in0_clocking_mmcm_read_write(1);
	while(!hdmi_in0_clocking_mmcm_drdy_read());
	return hdmi_in0_clocking_mmcm_dat_r_read();
}

MMCM hdmi_in0_clocking_mmcm = {
	.write = &hdmi_in0_clocking_mmcm_write,
	.read = &hdmi_in0_clocking_mmcm_read,
};
#endif

#ifdef CSR_HDMI_IN1_CLOCKING_MMCM_RESET_ADDR
void hdmi_in1_clocking_mmcm_write(int adr, int data)
{
	hdmi_in1_clocking_mmcm_adr_write(adr);
	hdmi_in1_clocking_mmcm_dat_w_write(data);
	hdmi_in1_clocking_mmcm_write_write(1);
	while(!hdmi_in1_clocking_mmcm_drdy_read());
}

int hdmi_in1_clocking_mmcm_read(int adr)
{
	hdmi_in1_clocking_mmcm_adr_write(adr);
	hdmi_in1_clocking_mmcm_read_write(1);
	while(!hdmi_in1_clocking_mmcm_drdy_read());
	return hdmi_in1_clocking_mmcm_dat_r_read();
}

MMCM hdmi_in1_clocking_mmcm = {
	.write = &hdmi_in1_clocking_mmcm_write,
	.read = &hdmi_in1_clocking_mmcm_read,
};
#endif

static void mmcm_config_30to60mhz(MMCM *mmcm)
{
	mmcm->write(0x14, 0x1000 | (10<<6) | 10); /* clkfbout_mult  = 20 */
	mmcm->write(0x08, 0x1000 | (10<<6) | 10); /* clkout0_divide = 20 */
	mmcm->write(0x0a, 0x1000 |  (8<<6) |  8); /* clkout1_divide = 16 */
	mmcm->write(0x0c, 0x1000 |  (2<<6) |  2); /* clkout2_divide =  4 */
	mmcm->write(0x0d, 0);                     /* clkout2_divide =  4 */
}

static void mmcm_config_60to120mhz(MMCM *mmcm)
{
	mmcm->write(0x14, 0x1000 |  (5<<6) | 5); /* clkfbout_mult  = 10 */
	mmcm->write(0x08, 0x1000 |  (5<<6) | 5); /* clkout0_divide = 10 */
	mmcm->write(0x0a, 0x1000 |  (4<<6) | 4); /* clkout1_divide =  8 */
	mmcm->write(0x0c, 0x1000 |  (1<<6) | 1); /* clkout2_divide =  2 */
	mmcm->write(0x0d, 0);                    /* clkout2_divide =  2 */
}

static void mmcm_config_120to240mhz(MMCM *mmcm)
{
	mmcm->write(0x14, 0x1000 |  (2<<6) | 3);  /* clkfbout_mult  = 5 */
	mmcm->write(0x08, 0x1000 |  (2<<6) | 3);  /* clkout0_divide = 5 */
	mmcm->write(0x0a, 0x1000 |  (2<<6) | 2);  /* clkout1_divide = 4 */
	mmcm->write(0x0c, 0x1000 |  (0<<6) | 0);  /* clkout2_divide = 1 */
	mmcm->write(0x0d, (1<<6));                /* clkout2_divide = 1 */
}

#define S7_MMCM_MAP_LEN  23
// map order comes from xapp888 -- no explanation in docs for why the order is necessary, but it seems important
static int addr_map[S7_MMCM_MAP_LEN] = {0x28, 0x9, 0x8, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0x10, 0x11, 0x6, 0x7,
					0x12, 0x13, 0x16, 0x14, 0x15, 0x18, 0x19, 0x1a, 0x4e, 0x4f};

void mmcm_dump_code(void) {
  int i;

  printf( "// hdmi0 out MMCM\n" );
  printf( "int hdmi0_mmcm_out[%d] = {", S7_MMCM_MAP_LEN * 2 );
  for( i = 0; i < S7_MMCM_MAP_LEN; i++ ) {
    if( addr_map[i] == 0x28 )  // this state substitutes to all 1's for DRP to work
      printf( "0x28, 0xffff" );
    else
      printf( "0x%x, 0x%x", addr_map[i], hdmi_out0_driver_clocking_mmcm_read(addr_map[i]) );
    if( i < S7_MMCM_MAP_LEN - 1 )
      printf( ", " );
  }
  printf( "};\n" );

  printf( "// hdmi1 out MMCM\n" );
  printf( "int hdmi1_mmcm_out[%d] = {", S7_MMCM_MAP_LEN * 2 );
  for( i = 0; i < S7_MMCM_MAP_LEN; i++ ) {
    if( addr_map[i] == 0x28 )  // this state substitutes to all 1's for DRP to work
      printf( "0x28, 0xffff" );
    else
      printf( "0x%x, 0x%x", addr_map[i], hdmi_out1_driver_clocking_mmcm_read(addr_map[i]) );
    if( i < S7_MMCM_MAP_LEN - 1 )
      printf( ", " );
  }
  printf( "};\n" );

  printf( "// hdmi0 MMCM\n" );
  printf( "int hdmi0_mmcm[%d] = {", S7_MMCM_MAP_LEN * 2 );
  for( i = 0; i < S7_MMCM_MAP_LEN; i++ ) {
    if( addr_map[i] == 0x28 )  // this state substitutes to all 1's for DRP to work
      printf( "0x28, 0xffff" );
    else
      printf( "0x%x, 0x%x", addr_map[i], hdmi_in0_clocking_mmcm_read(addr_map[i]) );
    if( i < S7_MMCM_MAP_LEN - 1 )
      printf( ", " );
  }
  printf( "};\n" );
  
  printf( "// hdmi1 MMCM\n" );
  printf( "int hdmi1_mmcm[%d] = {", S7_MMCM_MAP_LEN * 2 );
  for( i = 0; i < S7_MMCM_MAP_LEN; i++ ) {
    if( addr_map[i] == 0x28 )  // this state substitutes to all 1's for DRP to work
      printf( "0x28, 0xffff" );
    else
      printf( "0x%x, 0x%x", addr_map[i], hdmi_in1_clocking_mmcm_read(addr_map[i]) );
    if( i < S7_MMCM_MAP_LEN - 1 )
      printf( ", " );
  }
  printf( "};\n" );

}

/*
  "table" values are generated by doing a direct dump of the mmcm/plle2 values and recording them
  without going to the intermediate decoded values, which is error-prone during the recoding process...
 */
#define MTE 46  // MCM table entries
void mmcm_config_120to240mhz_table(MMCM *mmcm) {
  // according to xap888, register 0x28 should be all high when using DRP, so manually edit dumps accordingly
  
  // hdmi0 MMCM  (bandwidth = LOW)
  int hdmi0_mmcm_low[MTE] = {0x28, 0xffff, 0x9, 0x80, 0x8, 0x1083, 0xa, 0x1082, 0xb, 0x0, 0xc, 0x1041,
			     0xd, 0x40, 0xe, 0x41, 0xf, 0x40, 0x10, 0x41, 0x11, 0x40, 0x6, 0x41, 0x7, 0x40,
			     0x12, 0x41, 0x13, 0x40, 0x16, 0x1041, 0x14, 0x1083, 0x15, 0x80,
			     0x18, 0x3e8, 0x19, 0x3801, 0x1a, 0xbbe9, 0x4e, 0x808, 0x4f, 0x1900};
  
  // hdmi0 MMCM  (bandwidth = OPTIMIZED/HIGH)
  int hdmi0_mmcm_opt[MTE] = {0x28, 0xffff, 0x9, 0x80, 0x8, 0x1083, 0xa, 0x1082, 0xb, 0x0, 0xc, 0x1041,
			     0xd, 0x40, 0xe, 0x41, 0xf, 0x40, 0x10, 0x41, 0x11, 0x40, 0x6, 0x41, 0x7, 0x40,
			     0x12, 0x41, 0x13, 0x40, 0x16, 0x1041, 0x14, 0x1083, 0x15, 0x80,
			     0x18, 0x3e8, 0x19, 0x3801, 0x1a, 0xbbe9, 0x4e, 0x9108, 0x4f, 0x1900};

  int i;
  //  printf( "mmcm config %x\n", (unsigned int) mmcm );
  for( i = 0; i < MTE; i += 2 ) {
    //    printf( "%x: %x\n", hdmi0_mmcm_opt[i], hdmi0_mmcm_opt[i+1] );
    mmcm->write(hdmi0_mmcm_opt[i], hdmi0_mmcm_opt[i+1]);
  }
}

void mmcm_config_for_clock(MMCM *mmcm, int freq)
{
	/*
	 * FIXME: we also need to configure phase detector
	 */
	if(freq < 3000)
		wprintf("Frequency too low for input MMCMs\n");
	else if(freq < 6000)
		mmcm_config_30to60mhz(mmcm);
	else if(freq < 12000)
		mmcm_config_60to120mhz(mmcm);
	else if(freq < 24000)
		mmcm_config_120to240mhz_table(mmcm);
	else
		wprintf("Frequency too high for input MMCMs\n");
}

void mmcm_dump(MMCM* mmcm)
{
	int i;
	for(i=0;i<128;i++)
		printf("%04x ", mmcm->read(i));

}
void mmcm_dump_all(void)
{
	int i;
#ifdef CSR_HDMI_OUT0_DRIVER_CLOCKING_MMCM_RESET_ADDR
	printf("framebuffer MMCM:\n");
	mmcm_dump(&hdmi_out0_driver_clocking_mmcm);
	printf("\n");
#endif
#ifdef CSR_HDMI_IN0_CLOCKING_MMCM_RESET_ADDR
	printf("dvisampler MMCM:\n");
	mmcm_dump(&hdmi_in0_clocking_mmcm);
	printf("\n");
#endif
}
