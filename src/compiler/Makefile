.PHONY:		all clean
.PHONY:		test tests valgrind leak leaks

all:
		$(MAKE) -C ptest

# ----- Tests -----------------------------------------------------------------

test tests:     all
		LANG= sh -c						\
		    'passed=0 && failed=0 && cd test &&			\
		    for n in [a-z]*; do					\
		    [ $$n != core ] && SCRIPT=$$n . ./$$n; done;	\
		    if [ $$failed = 0 ]; then				\
		        echo "Passed all $$passed tests";		\
		    else						\
			total=`expr $$passed + $$failed`;		\
		        echo "Failed $$failed/$$total tests";		\
			exit 1;						\
		    fi'

valgrind:
		VALGRIND="valgrind -q" $(MAKE) tests

leak leaks:
		VALGRIND="valgrind -q --leak-check=full --show-reachable=yes" \
		    FAIL_ON_ERROR=false $(MAKE) tests

# ----- Cleanup ---------------------------------------------------------------

clean:
		$(MAKE) -C ptest clean
		rm -f test/core
