FILES=$(shell ls *.c | sed 's,\.c,.run,g')
GCC=/usr/bin/gcc
CFLAGS=-g -std=gnu99 -Wall -fPIC -pthread -O2
LOCLIBS=-L.. -lGSDPRM

.PHONY: test
test: $(FILES)
	for i in *.run; do \
		echo "Running $$i.."; \
		LD_LIBRARY_PATH=".." ./$$i; \
		echo; \
	done

.PHONY: testgrind
CFLAGS := $(CFLAGS) -O0
testgrind: $(FILES)
	for i in *.run; do \
	    echo "Running $$i.."; \
	    LD_LIBRARY_PATH=".." valgrind --leak-check=full \
	                                  --read-var-info=yes \
	                                  --track-origins=yes \
	                        ./$$i; \
	    echo; \
	done

%.run: %.c
	$(GCC) $(CFLAGS) $< $(LOCLIBS) -o $@

clean:
	rm -rf *.run
