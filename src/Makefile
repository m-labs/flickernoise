CC=lm32-rtems4.11-gcc
LD=lm32-rtems4.11-gcc
STRIP=lm32-rtems4.11-strip
OBJCOPY=lm32-rtems4.11-objcopy

CFLAGS=-O9 -Wall -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -fsingle-precision-constant -I$(RTEMS_MAKEFILE_PATH)/lib/include
LDFLAGS=-mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -B$(RTEMS_MAKEFILE_PATH)/lib -specs bsp_specs -qrtems
STRIPFLAGS=

# base
OBJS=shellext.o sysconfig.o config.o fb.o input.o reboot.o osc.o pngload.o flashvalid.o main.o

# GUI
OBJS+=messagebox.o filedialog.o resmgr.o guirender.o performance.o cp.o keyboard.o ir.o audio.o midi.o oscsettings.o dmxspy.o dmxtable.o dmx.o videoin.o patcheditor.o monitor.o firstpatch.o pdfreader.o sysettings.o about.o flash.o shutdown.o

# renderer
OBJS+=framedescriptor.o analyzer.o sampler.o compiler.o eval.o line.o wave.o font.o osd.o raster.o renderer.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -lnfs -lftpd -ltelnetd -lyaffs2 -lmtk -llop -lfpvm -lpng -lmupdf -lfreetype -ljbig2dec -lopenjpeg -ljpeg -lz -lm
	$(STRIP) $(STRIPFLAGS) $@

bandfilters.h: bandfilters.sce
	scilab -nw -nwni -nogui -nb -f bandfilters.sce

# boot images for Milkymist
flickernoise.ralf: flickernoise
	$(OBJCOPY) -O binary flickernoise flickernoise.ralf

flickernoise.fbi: flickernoise.lzma
	mkmmimg flickernoise.lzma write flickernoise.fbi

flickernoise.lzma: flickernoise.ralf
	cat flickernoise.ralf | lzma > flickernoise.lzma

flickernoise.fbiz: flickernoise.lzma
	mkmmimg flickernoise.lzma writelzma flickernoise.fbiz

# convenience target for loading to MM board
load: flickernoise.ralf
	cp flickernoise.ralf /var/lib/tftpboot/boot.bin

clean:
	rm -f flickernoise flickernoise.ralf flickernoise.lzma flickernoise.fbi flickernoise.fbiz $(OBJS)

.PHONY: clean load
