FILES=$(shell ls *.c | sed 's,\.c,.run,g')
GCC=/usr/bin/gcc
CFLAGS=-g -std=gnu99 -Wall -fPIC -pthread -lunistring
EXTLIBS=-L../GSD_Dictionary/src -lGSDDict
LOCLIBS=-L.. -lGSDGC

test: $(FILES)
	for i in *.run; do \
		echo "Running $$i.."; \
		LD_LIBRARY_PATH=".." ./$$i; \
		echo; \
	done

%.run: %.c
	$(GCC) -O2 $(CFLAGS) $< $(EXTLIBS) $(LOCLIBS) -o $@

clean:
	rm -rf *.run
