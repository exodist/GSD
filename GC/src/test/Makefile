FILES=$(shell ls *.c | sed 's,\.c,.run,g')
GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC -pthread -O2
LOCLIBS=-L.. -lGSDGC

.PHONY: test
test: $(FILES)
	for i in *.run; do \
		echo "Running $$i.."; \
		LD_LIBRARY_PATH=".." ./$$i; \
		echo; \
	done

.PHONY: testgrind
testgrind: CFLAGS := $(CFLAGS) -g -O0
testgrind: $(FILES)
	for i in *.run; do \
	    echo "Running $$i.."; \
	    LD_LIBRARY_PATH=".." valgrind --leak-check=full \
									  --max-stackframe=8008048 \
	                                  --read-var-info=yes \
	                                  --track-origins=yes \
	                        ./$$i; \
	    echo; \
	done

.PHONY: testgdb
testgdb: CFLAGS := $(CFLAGS) -g -O0
testgdb: $(FILES)
	for i in *.run; do \
	    echo "Running $$i.."; \
	    LD_LIBRARY_PATH=".." gdb ./$$i; \
	    echo; \
	done

.PHONY: testgdbgrind
testgdbgrind: CFLAGS := $(CFLAGS) -g -O0
testgdbgrind: $(FILES)
	for i in *.run; do \
	    echo "Running $$i.."; \
	    LD_LIBRARY_PATH=".." valgrind --leak-check=full \
	                                  --read-var-info=yes \
	                                  --track-origins=yes \
									  --vgdb=yes \
									  --vgdb-error=0 \
	                        ./$$i; \
	    echo; \
	done

%.run: %.c
	$(GCC) $(CFLAGS) $< $(LOCLIBS) -o $@

clean:
	rm -rf *.run *.out

.PHONY: testprof
testprof: CFLAGS := $(CFLAGS) -g -O0
testprof: $(FILES)
	for i in *.run; do \
	    echo "Running $$i.."; \
	    LD_LIBRARY_PATH=".." valgrind --tool=callgrind \
									  --callgrind-out-file=$$i.callgrind.out \
	                        ./$$i; \
	    echo; \
	done


