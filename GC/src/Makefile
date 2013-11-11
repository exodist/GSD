GCC=/usr/bin/gcc
CFLAGS := -O2 -std=gnu99 -Wall -fPIC -pthread

libGSDGC.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDGC.so.1.0.0 -o libGSDGC.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1.0
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	rm -rf include/*.gch
	$(MAKE) -C test clean

.PHONY: test
test: libGSDGC.so
	$(MAKE) -C test test

.PHONY: testgrind
testgrind: CFLAGS := $(CFLAGS) -g -O0
testgrind: libGSDGC.so
	$(MAKE) -C test testgrind

.PHONY: testprof
testprof: CFLAGS := $(CFLAGS) -g -O0
testprof: libGSDGC.so
	$(MAKE) -C test testprof

.PHONY: testgdb
testgdb: CFLAGS := $(CFLAGS) -g -O0
testgdb: libGSDGC.so
	$(MAKE) -C test testgdb

.PHONY: testgdbgrind
testgdbgrind: CFLAGS := $(CFLAGS) -g -O0
testgdbgrind: libGSDGC.so
	$(MAKE) -C test testgdbgrind

