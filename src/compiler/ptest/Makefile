MMDIR ?= ../../../../milkymist
FPVM_H = $(MMDIR)/software/include/fpvm
LIBFPVM_X86 = $(MMDIR)/software/libfpvm/x86-linux

CFLAGS_STANDALONE = -DSTANDALONE=\"standalone.h\"
CFLAGS = -Wall -g -I.. -I. $(CFLAGS_STANDALONE)
OBJS = ptest.o scanner.o parser.o parser_helper.o unique.o compiler.o \
       libfpvm.a
LDLIBS = -lm

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
		$(GEN) ln -s $(FPVM_H) fpvm

ptest:		$(OBJS)
		$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o:		../%.c
		$(CC) $(CFLAGS) -c -o $@ $<

%.c:		%.re
		$(GEN) re2c -c -o $@ $<

%.c:		%.y
		$(GEN) lemon $<

%.h %.inc:	%.ids
		$(GEN) cd .. && ./idgen `basename $<`

../infra-fnp.h:	../finish-pfv.fnp ../init-pvv.fnp ../finish-pvv.fnp
		$(GEN) ../file2h -c $^ >$@ || { rm -f $@; }

libfpvm.a:
		$(MAKE) -C $(LIBFPVM_X86)
		cp $(LIBFPVM_X86)/$@ .

# ----- Dependencies ----------------------------------------------------------

../parser.h:	../parser.c
scanner.o:	../parser.h
parser_helper.o: ../parser.h
ptest.o:	../parser.h
unique.o:	../fnp.inc
compiler.o:	../infra-fnp.h

# ----- Cleanup ---------------------------------------------------------------

clean:
		$(MAKE) -C $(LIBFPVM_X86) clean
		rm -f $(OBJS)
		rm -f ../scanner.c
		rm -f ../parser.c ../parser.h ../parser.out
		rm -f ../fnp.h ../fnp.inc
		rm -f ../infra-fnp.h libfpvm.a
		rm -f fpvm
