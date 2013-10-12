GCC=/usr/bin/gcc
CFLAGS := -O2 -std=gnu99 -Wall -fPIC -pthread

libGSDPRM.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDPRM.so.1.0.0 -o libGSDPRM.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so.1.0
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so.1
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so

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
test: libGSDPRM.so
	$(MAKE) -C test test

.PHONY: testgrind
testgrind: CFLAGS := $(CFLAGS) -g -O0
testgrind: libGSDPRM.so
	$(MAKE) -C test testgrind

.PHONY: testgdb
testgdb: CFLAGS := $(CFLAGS) -g -O0
testgdb: libGSDPRM.so
	$(MAKE) -C test testgdb

.PHONY: testgdbgrind
testgdbgrind: CFLAGS := $(CFLAGS) -g -O0
testgdbgrind: libGSDPRM.so
	$(MAKE) -C test testgdbgrind

