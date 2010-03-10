include config.mak

OBJS=main.o

all: flickernoise

flickernoise: $(OBJS)
	$(LD) -o $@ $< $(LDFLAGS) -ldope
	$(STRIP) $(STRIPFLAGS) $@

clean:
	rm -f flickernoise $(OBJS)

.PHONY: clean
