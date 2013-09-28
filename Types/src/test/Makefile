FILES=$(shell ls *.c | sed 's,.c,.run,g')
GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC
EXTLIBS=-pthread -lunistring -L../GSD_Dictionary/src/ -lGSDDict
LOCLIBS=-L../ -lGSDGC

test: $(FILES)
	for i in *.run; do \
		echo "Running $$i.."; \
		./$$i; \
		echo; \
	done

%.run: %.c
	$(GCC) -O2 $(CFLAGS) $(EXTLIBS) $(LOCLIBS) $< -o $@

clean:
	rm -rf *.run
