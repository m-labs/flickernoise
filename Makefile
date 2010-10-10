include config.mak

OBJS=filedialog.o main.o cp.o patcheditor.o monitor.o shutdown.o flash.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -ldope
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
	flterm --port /dev/ttyUSB0 --kernel flickernoise.ralf

clean:
	rm -f flickernoise flickernoise.ralf flickernoise.fbi flickernoise-rescue.mcs flickernoise.mcs $(OBJS)

.PHONY: clean load
