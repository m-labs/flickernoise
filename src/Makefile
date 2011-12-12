RTEMS_VERSION ?= 4.11
RTEMS_MAKEFILE_PATH ?= \
    /opt/rtems-$(RTEMS_VERSION)/lm32-rtems$(RTEMS_VERSION)/milkymist/
WITH_PDF ?= 0

CROSS_COMPILER=lm32-rtems$(RTEMS_VERSION)-
CC_normal := $(CROSS_COMPILER)gcc
LD_normal := $(CROSS_COMPILER)gcc
OBJCOPY_normal := $(CROSS_COMPILER)objcopy
CC_quiet = @echo " CC " $@ && $(CROSS_COMPILER)gcc
LD_quiet = @echo " LD " $@ && $(CROSS_COMPILER)gcc
OBJCOPY_quiet = @echo " OBJCOPY " $@ && $(CROSS_COMPILER)objcopy

ifeq ($(V),1)
    CC = $(CC_normal)
    AR = $(AR_normal)
    AS = $(AS_normal)
    LD = $(LD_normal)
    OBJCOPY = $(OBJCOPY_normal)
    RANLIB  = $(RANLIB_normal)
else
    CC = $(CC_quiet)
    AR = $(AR_quiet)
    AS = $(AS_quiet)
    LD = $(LD_quiet)
    OBJCOPY = $(OBJCOPY_quiet)
    RANLIB  = $(RANLIB_quiet)
endif

CFLAGS = -O9 -Wall -Wstrict-prototypes -Wmissing-prototypes \
	 -mbarrel-shift-enabled -mmultiply-enabled \
	 -mdivide-enabled -msign-extend-enabled -fsingle-precision-constant \
	 -I$(RTEMS_MAKEFILE_PATH)/lib/include -g
LDFLAGS = -mbarrel-shift-enabled -mmultiply-enabled -mdivide-enabled \
	  -msign-extend-enabled -B$(RTEMS_MAKEFILE_PATH)/lib \
	  -specs bsp_specs -qrtems
LIBS = -lnfs -lftpd -ltelnetd -lyaffs2 -lmtk -llop -lfpvm -lpng -ljpeg \
      -lcurl -lexpat -lz -lm
OBJDIR = obj
BINDIR = bin

OBJS = yaffs.o version.o shellext.o sysconfig.o config.o fb.o input.o \
       keymap.o fbgrab.o shortcuts.o osc.o pngwrite.o patchpool.o \
       flashvalid.o main.o
OBJS += $(addprefix pixbuf/,dither.o loaderjpeg.o loaderpng.o manager.o)
OBJS += $(addprefix gui/,messagebox.o filedialog.o resmgr.o guirender.o \
	performance.o cp.o keyboard.o ir.o audio.o midi.o oscsettings.o \
	dmxspy.o dmxdesk.o dmx.o videoin.o rsswall.o patcheditor.o monitor.o \
	firstpatch.o filemanager.o sysettings.o about.o flash.o shutdown.o)
ifeq ($(WITH_PDF),1)
	OBJS += $(addprefix gui/,pdfreader.o)
	CFLAGS += -DWITH_PDF
	LIBS := -lmupdf -lfreetype -ljbig2dec -lopenjpeg $(LIBS)
endif
OBJS += $(addprefix translations/,french.o german.o)
OBJS += $(addprefix renderer/,framedescriptor.o analyzer.o sampler.o \
	eval.o line.o wave.o font.o osd.o raster.o renderer.o \
	videoinreconf.o)
OBJS += $(addprefix compiler/,compiler.o fpvm.o parser_helper.o scanner.o \
	parser.o unique.o)

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

bandfilters.h: bandfilters.sce
	scilab -nw -nwni -nogui -nb -f bandfilters.sce

%.c: %.re
	re2c -c -o $@ $<

%.c: %.y
	lemon $<

%.h %.inc: %.ids
	cd compiler && ./idgen `basename $<`

compiler/infra-fnp.h: \
	  compiler/finish-pfv.fnp compiler/init-pvv.fnp compiler/finish-pvv.fnp
	compiler/file2h $^ >$@ || { rm -f $@; }

compiler/parser.h: compiler/parser.c
obj/compiler/scanner.o: compiler/parser.h
obj/compiler/parser_helper.o: compiler/parser.h
obj/compiler/fpvm.o: compiler/parser.h
obj/compiler/unique.o: compiler/fnp.inc
obj/compiler/compiler.o: compiler/infra-fnp.h compiler/parser.h

# boot images for Milkymist One
$(BINDIR)/flickernoise.bin: $(BINDIR)/flickernoise
	$(OBJCOPY) -O binary $< $@

$(BINDIR)/flickernoise.fbi: $(BINDIR)/flickernoise.bin
	mkmmimg $< write $@

$(BINDIR)/flickernoise.lzma: $(BINDIR)/flickernoise.bin
	cat $< | lzma > $@

$(BINDIR)/flickernoise.fbiz: $(BINDIR)/flickernoise.lzma
	mkmmimg $< writelzma $@

# convenience target for loading to MM board
load: $(BINDIR)/flickernoise.bin
	cp $< /var/lib/tftpboot/boot.bin

flash:
	(cd ../flash && ./flash.sh rescue)
	(cd ../flash && ./flash.sh)

clean:
	rm -f $(BINDIR)/flickernoise $(BINDIR)/flickernoise.bin 
	rm -f $(BINDIR)/flickernoise.lzma $(BINDIR)/flickernoise.fbi \
	    $(BINDIR)/flickernoise.fbiz 
	rm -f $(POBJS)
	rm -f compiler/scanner.c
	rm -f compiler/parser.c compiler/parser.h compiler/parser.out
	rm -f compiler/fnp.h compiler/fnp.inc compiler/infra-fnp.h

.PHONY: clean load flash
