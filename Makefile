CC=lm32-rtems4.11-gcc
LD=lm32-rtems4.11-gcc
STRIP=lm32-rtems4.11-strip
OBJCOPY=lm32-rtems4.11-objcopy

GENODEFX?=../genode-fx

CFLAGS=-O9 -Wall -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -fsingle-precision-constant -I$(GENODEFX)/dope-embedded/include -I$(RTEMS_MAKEFILE_PATH)/lib/include
LDFLAGS=-L$(GENODEFX)/dope-embedded/lib/milkymist -B$(RTEMS_MAKEFILE_PATH)/lib -specs bsp_specs -qrtems
STRIPFLAGS=

# base
OBJS=main.o

# GUI
OBJS+=filedialog.o guirender.o cp.o audio.o patcheditor.o monitor.o about.o flash.o shutdown.o

# renderer
OBJS+=framedescriptor.o analyzer.o sampler.o compiler.o eval.o line.o wave.o raster.o renderer.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -lnfs -ldope -lfpvm -lm
	$(STRIP) $(STRIPFLAGS) $@

bandfilters.h: bandfilters.sce
	scilab -nw -nwni -nogui -nb -f bandfilters.sce

analyzer.c: bandfilters.h

# boot images for Milkymist
flickernoise.ralf: flickernoise
	$(OBJCOPY) -O binary flickernoise flickernoise.ralf

flickernoise.fbi: flickernoise.ralf
	mkmmimg flickernoise.ralf write flickernoise.fbi

flickernoise-rescue.mcs: flickernoise.fbi
	srec_cat -Output flickernoise-rescue.mcs -Intel flickernoise.fbi -Binary -offset 0x002E0000 --byte-swap 2

flickernoise.mcs: flickernoise.fbi
	srec_cat -Output flickernoise.mcs -Intel flickernoise.fbi -Binary -offset 0x00920000 --byte-swap 2

# convenience target for loading to MM board
load: flickernoise.ralf
	sudo cp flickernoise.ralf /milkymist/boot.bin

clean:
	rm -f bandfilters.h flickernoise flickernoise.ralf flickernoise.fbi flickernoise-rescue.mcs flickernoise.mcs $(OBJS)

.PHONY: clean load
