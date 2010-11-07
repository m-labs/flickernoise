CC=lm32-rtems4.11-gcc
LD=lm32-rtems4.11-gcc
STRIP=lm32-rtems4.11-strip
OBJCOPY=lm32-rtems4.11-objcopy

MTK?=../mtk

CFLAGS=-O9 -Wall -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -fsingle-precision-constant -I$(MTK)/include -I$(RTEMS_MAKEFILE_PATH)/lib/include
LDFLAGS=-mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -L$(MTK)/lib/milkymist -B$(RTEMS_MAKEFILE_PATH)/lib -specs bsp_specs -qrtems
STRIPFLAGS=

# base
OBJS=config.o fb.o input.o reboot.o main.o

# GUI
OBJS+=messagebox.o filedialog.o resmgr.o guirender.o cp.o keyboard.o audio.o dmxtable.o dmx.o patcheditor.o monitor.o firstpatch.o sysettings.o about.o flash.o shutdown.o

# renderer
OBJS+=framedescriptor.o analyzer.o sampler.o compiler.o eval.o line.o wave.o raster.o renderer.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -lnfs -lmtk -lfpvm -lm
	$(STRIP) $(STRIPFLAGS) $@

bandfilters.h: bandfilters.sce
	scilab -nw -nwni -nogui -nb -f bandfilters.sce

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
	cp flickernoise.ralf /var/lib/tftpboot/boot.bin

clean:
	rm -f flickernoise flickernoise.ralf flickernoise.fbi flickernoise-rescue.mcs flickernoise.mcs $(OBJS)

.PHONY: clean load
