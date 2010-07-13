include config.mak

OBJS=filedialog.o main.o patcheditor.o monitor.o shutdown.o flash.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -ldope 
	$(STRIP) $(STRIPFLAGS) $@

clean:
	rm -f flickernoise $(OBJS)

.PHONY: clean
