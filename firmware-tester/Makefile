NETV2_DIR=../build

include $(NETV2_DIR)/software/include/generated/variables.mak
include $(SOC_DIRECTORY)/software/common.mak

ifeq ($(OS),Windows_NT)
COPY := cmd /c copy
else
COPY := cp
endif

OBJECTS=bist.o \
	ci.o \
	config.o \
	edid.o \
	encoder.o \
	fx2.o \
	hdmi_in0.o \
	hdmi_out0.o \
	hdmi_out1.o \
	heartbeat.o \
	i2c.o \
	isr.o \
	main.o \
	mdio.o \
	mmcm.o \
	oled.o \
	opsis_eeprom.o \
	pattern.o \
	pll.o \
	processor.o \
	reboot.o \
	stdio_wrap.o \
	uptime.o \
	version.o \
	dump.o \
	version_data.o \
	hdmi_in1.o \
	tester.o \
        sdcard.o

#	boot-helper-$(CPU).o
#	telnet.o \
#	tofe_eeprom.o \
#	etherbone.o \
#	ethernet.o \

CFLAGS += -I. \
	-I../third_party/libuip \
	-I./uip \

CFLAGS += -Wall \
#	-Werror \
#	-Wno-error=unused-function \
#	-Wno-error=unused-variable \

all: firmware.bin

# pull in dependency info for *existing* .o files
-include $(OBJECTS:.o=.d)

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@
ifneq ($(OS),Windows_NT)
	chmod -x $@
endif
	$(COPY) $@ boot.bin

firmware.elf: $(LINKER_LD) $(OBJECTS)

LD=$(TARGET_PREFIX)ld

firmware.elf: ../build/software/libbase/libbase-nofloat.a ../build/software/libcompiler_rt/libcompiler_rt.a \
	../build/software/bios/boot-helper-vexriscv.o $(OBJECTS)
	$(LD) $(LDFLAGS) \
		-T linker-ctr.ld \
		-N -o $@ \
		 $(NETV2_DIR)/software/libbase/crt0-$(CPU)-ctr.o \
		$(OBJECTS) \
	../build/software/bios/boot-helper-vexriscv.o \
		-L$(NETV2_DIR)/software/libbase \
		-L$(NETV2_DIR)/software/libcompiler_rt \
		-lbase-nofloat -lcompiler_rt
	chmod -x $@

ifneq ($(OS),Windows_NT)
	chmod -x $@
endif

main.o: main.c
	$(compile)

%.o: %.c
	$(compile)

%.o: %.S
	$(assemble)

load: firmware.bin
	litex_term --kernel firmware.bin COM8

clean:
	$(RM) $(OBJECTS) $(OBJECTS:.o=.d) firmware.elf firmware.bin .*~ *~


# Dependencies on generated files
ci.o: hdmi_in1.h
hdmi_in1.o: hdmi_in1.h hdmi_in1.c
pattern.o: version_data.h version_data.c
version.o: version_data.h version_data.c
version_data.o: version_data.h version_data.c

# Generated version info
version_data.h: version_data
version_data.c: version_data
version_data:
	bash version_data.sh
	# Check the version files exist
	[ -e version_data.h ]
	[ -e version_data.c ]

# Generated hdmi_in1 from hdmi_in0
hdmi_in1.h: hdmi_in0.h hdmi_in.sh
	bash hdmi_in.sh 1
	# Check the files exist
	[ -e hdmi_in1.h ]

hdmi_in1.c: hdmi_in0.c hdmi_in.sh
	bash hdmi_in.sh 1
	# Check the files exist
	[ -e hdmi_in1.c ]

.PHONY: all clean libs version_data

