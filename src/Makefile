RTEMS_VERSION?=4.11
RTEMS_MAKEFILE_PATH?=/opt/rtems-$(RTEMS_VERSION)/lm32-rtems$(RTEMS_VERSION)/milkymist/
WITH_PDF?=0

CC=lm32-rtems$(RTEMS_VERSION)-gcc
LD=lm32-rtems$(RTEMS_VERSION)-gcc
STRIP=lm32-rtems$(RTEMS_VERSION)-strip
OBJCOPY=lm32-rtems$(RTEMS_VERSION)-objcopy

CFLAGS=-O9 -Wall -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -fsingle-precision-constant -I$(RTEMS_MAKEFILE_PATH)/lib/include
LDFLAGS=-mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled -msign-extend-enabled -B$(RTEMS_MAKEFILE_PATH)/lib -specs bsp_specs -qrtems
STRIPFLAGS=
LIBS=-lnfs -lftpd -ltelnetd -lyaffs2 -lmtk -llop -lfpvm -lpng -lz -lm
OBJDIR=obj
BINDIR=bin

# base
OBJS=shellext.o sysconfig.o config.o fb.o input.o fbgrab.o shortcuts.o osc.o pngload.o flashvalid.o main.o

# GUI
OBJS+=messagebox.o filedialog.o resmgr.o guirender.o performance.o cp.o keyboard.o ir.o audio.o midi.o oscsettings.o dmxspy.o dmxtable.o dmx.o videoin.o patcheditor.o monitor.o firstpatch.o filemanager.o sysettings.o about.o flash.o shutdown.o

# PDF
ifeq ($(WITH_PDF),1)
	OBJS+=pdfreader.o
	CFLAGS+=-DWITH_PDF
	LIBS:=-lmupdf -lfreetype -ljbig2dec -lopenjpeg -ljpeg $(LIBS)
endif

# renderer
OBJS+=framedescriptor.o analyzer.o sampler.o compiler.o eval.o line.o wave.o font.o osd.o raster.o renderer.o

#

POBJS=$(addprefix $(OBJDIR)/,$(OBJS))

all: $(BINDIR)/flickernoise

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# always rebuild about.o to show the latest build date
.PHONY: $(OBJDIR)/about.o
$(OBJDIR)/about.o: about.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(BINDIR)/flickernoise: $(POBJS)
	$(LD) -o $@ $^ $(LDFLAGS) $(LIBS)
	$(STRIP) $(STRIPFLAGS) $@

bandfilters.h: bandfilters.sce
	scilab -nw -nwni -nogui -nb -f bandfilters.sce

# boot images for Milkymist One
$(BINDIR)/flickernoise.ralf: $(BINDIR)/flickernoise
	$(OBJCOPY) -O binary $< $@

$(BINDIR)/flickernoise.fbi: $(BINDIR)/flickernoise.ralf
	mkmmimg $< write $@

$(BINDIR)/flickernoise.lzma: $(BINDIR)/flickernoise.ralf
	cat $< | lzma > $@

$(BINDIR)/flickernoise.fbiz: $(BINDIR)/flickernoise.lzma
	mkmmimg $< writelzma $@

# convenience target for loading to MM board
load: $(BINDIR)/flickernoise.ralf
	cp $(BINDIR)/flickernoise.ralf /var/lib/tftpboot/boot.bin

clean:
	rm -f $(BINDIR)/flickernoise $(BINDIR)/flickernoise.ralf 
	rm -f $(BINDIR)/flickernoise.lzma $(BINDIR)/flickernoise.fbi $(BINDIR)/flickernoise.fbiz 
	rm -f $(POBJS)

.PHONY: clean load
