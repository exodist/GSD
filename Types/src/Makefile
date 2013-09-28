GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC
EXTLIBS=-pthread -lunistring -LGSD_Dictionary/src/ -lGSDDict
LOCLIBS=-L. -lGSDGC

libGSDGC.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDGC.so.1.0.0 -o libGSDGC.so.1.0.0 *.o
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1.0
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1
	ln -sd libGSDGC.so.1.0.0 libGSDGC.so

libGSDDict.so: *.o
	$(MAKE) -C GSD_Dictionary/src/ libGSDDict.so

*.o:
	$(GCC) $(CFLAGS) -O2 -c *.c

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	$(MAKE) -C GSD_Dictionary/src/ clean
	$(MAKE) -C test clean

.PHONY: test
test: libGSDGC.so libGSDDict.so
	$(MAKE) -C test
