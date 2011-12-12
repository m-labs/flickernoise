RTEMS_VERSION ?= 4.11
RTEMS_MAKEFILE_PATH ?= \
    /opt/rtems-$(RTEMS_VERSION)/lm32-rtems$(RTEMS_VERSION)/milkymist/
RTEMS_FPVM_H = $(RTEMS_MAKEFILE_PATH)/lib/include/fpvm

CFLAGS = -Wall -g -I.. -I.
OBJS = ptest.o scanner.o parser.o parser_helper.o unique.o

# ----- Verbosity control -----------------------------------------------------

CC_normal	:= $(CC)

CC_quiet	= @echo "  CC       " $@ && $(CC_normal)
GEN_quiet       = @echo "  GENERATE " $@ &&

ifeq ($(V),1)
    CC		= $(CC_normal)
    GEN		=
else
    CC		= $(CC_quiet)
    GEN		= $(GEN_quiet)
endif

# ----- Rules -----------------------------------------------------------------

.PHONY:		all clean

all:		fpvm ptest

fpvm:
		$(GEN) ln -s $(RTEMS_FPVM_H) fpvm

ptest:		$(OBJS)
		$(CC) $(CFLAGS) -o $@ $^

%.o:		../%.c
		$(CC) $(CFLAGS) -c -o $@ $<

%.c:		%.re
		$(GEN) re2c -c -o $@ $<

%.c:		%.y
		$(GEN) lemon $<

%.h %.inc:	%.ids
		$(GEN) cd .. && ./idgen `basename $<`

# ----- Dependencies ----------------------------------------------------------

../parser.h:	../parser.c
scanner.o:	../parser.h
parser_helper.o: ../parser.h
ptest.o:	../parser.h
unique.o:	../fnp.inc

# ----- Cleanup ---------------------------------------------------------------

clean:
		rm -f $(OBJS)
		rm -f ../scanner.c
		rm -f ../parser.c ../parser.h ../parser.out
		rm -f ../fnp.h ../fnp.inc
		rm -f fpvm
