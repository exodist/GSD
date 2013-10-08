GCC=/usr/bin/gcc
CFLAGS=-g -std=gnu99 -Wall -fPIC -pthread

libGSDPRM.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDPRM.so.1.0.0 -o libGSDPRM.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so.1.0
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so.1
	ln -sf libGSDPRM.so.1.0.0 libGSDPRM.so

%.o: %.c
	$(GCC) $(CFLAGS) -O2 -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	rm -rf include/*.gch
	$(MAKE) -C test clean

.PHONY: test
test: libGSDPRM.so
	$(MAKE) -C test
