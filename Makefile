CC=lm32-rtems4.11-gcc
LD=lm32-rtems4.11-gcc
STRIP=lm32-rtems4.11-strip
OBJCOPY=lm32-rtems4.11-objcopy

GENODEFX=../genode-fx

CFLAGS=-O2 -Wall -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -I$(GENODEFX)/dope-embedded/include -I$(RTEMS_MAKEFILE_PATH)/lib/include
LDFLAGS=-L$(GENODEFX)/dope-embedded/lib/milkymist -B$(RTEMS_MAKEFILE_PATH)/lib -specs bsp_specs -qrtems
STRIPFLAGS=

OBJS=filedialog.o main.o cp.o audio.o patcheditor.o monitor.o about.o flash.o shutdown.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -ldope -lfpvm
	$(STRIP) $(STRIPFLAGS) $@

# boot images for Milkymist
flickernoise.ralf: flickernoise
	$(OBJCOPY) -O binary flickernoise flickernoise.ralf

flickernoise.fbi: flickernoise.ralf
	$(MMDIR)/tools/crc32 flickernoise.ralf write flickernoise.fbi

flickernoise-rescue.mcs: flickernoise.fbi
	srec_cat -Output flickernoise-rescue.mcs -Intel flickernoise.fbi -Binary -offset 0x002E0000 --byte-swap 2

flickernoise.mcs: flickernoise.fbi
	srec_cat -Output flickernoise.mcs -Intel flickernoise.fbi -Binary -offset 0x00920000 --byte-swap 2

# convenience target for loading to MM board
load: flickernoise.ralf
	sudo cp flickernoise.ralf /milkymist/boot.bin

clean:
	rm -f flickernoise flickernoise.ralf flickernoise.fbi flickernoise-rescue.mcs flickernoise.mcs $(OBJS)

.PHONY: clean load
