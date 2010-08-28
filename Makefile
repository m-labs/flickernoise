include config.mak

OBJS=filedialog.o main.o patcheditor.o monitor.o shutdown.o flash.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -ldope 
	$(STRIP) $(STRIPFLAGS) $@

flickernoise.ralf: flickernoise
	$(OBJCOPY) -O binary flickernoise flickernoise.ralf

# convenience target for loading to MM board
load: flickernoise.ralf
	flterm --port /dev/ttyUSB0 --kernel flickernoise.ralf

clean:
	rm -f flickernoise flickernoise.ralf $(OBJS)

.PHONY: clean load
