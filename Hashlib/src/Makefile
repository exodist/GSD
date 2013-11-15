GCC=/usr/bin/gcc
CFLAGS := -O2 -std=gnu99 -Wall -fPIC -pthread

libGSDHashlib.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDHashlib.so.1.0.0 -o libGSDHashlib.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDHashlib.so.1.0.0 libGSDHashlib.so.1.0
	ln -sf libGSDHashlib.so.1.0.0 libGSDHashlib.so.1
	ln -sf libGSDHashlib.so.1.0.0 libGSDHashlib.so

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	rm -rf include/*.gch
	$(MAKE) -C test clean || true

.PHONY: test
test: libGSDHashlib.so
	$(MAKE) -C test test

.PHONY: testgrind
testgrind: CFLAGS := $(CFLAGS) -g -O0
testgrind: libGSDHashlib.so
	$(MAKE) -C test testgrind

.PHONY: testprof
testprof: CFLAGS := $(CFLAGS) -g -O0
testprof: libGSDHashlib.so
	$(MAKE) -C test testprof

.PHONY: testgdb
testgdb: CFLAGS := $(CFLAGS) -g -O0
testgdb: libGSDHashlib.so
	$(MAKE) -C test testgdb

.PHONY: testgdbgrind
testgdbgrind: CFLAGS := $(CFLAGS) -g -O0
testgdbgrind: libGSDHashlib.so
	$(MAKE) -C test testgdbgrind

