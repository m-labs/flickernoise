.PHONY:		all clean
.PHONY:		test tests valgrind

all:
		$(MAKE) -C ptest

# ----- Tests -----------------------------------------------------------------

test tests:     all
		LANG= sh -c \
		    'passed=0 && cd test && \
		    for n in [a-z]*; do \
		    [ $$n != core ] && SCRIPT=$$n . ./$$n; done; \
		    echo "Passed all $$passed tests"'

valgrind:
		VALGRIND="valgrind -q" $(MAKE) tests

# ----- Cleanup ---------------------------------------------------------------

clean:
		$(MAKE) -C ptest clean
		rm -f test/core
