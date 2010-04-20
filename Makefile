include config.mak

OBJS=main.o patcheditor.o shutdown.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $(OBJS) $(LDFLAGS) -ldope
	$(STRIP) $(STRIPFLAGS) $@

clean:
	rm -f flickernoise $(OBJS)

.PHONY: clean
