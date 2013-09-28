GCC=/usr/bin/gcc
CFLAGS=-g -std=gnu99 -Wall -fPIC
EXTLIBS=-pthread -lunistring -LGSD_Dictionary/src -lGSDDict
LOCLIBS=-L. -lGSDGC

libGSDGC.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDGC.so.1.0.0 -o libGSDGC.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1.0
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so.1
	ln -sf libGSDGC.so.1.0.0 libGSDGC.so

libGSDDict.so:
	$(MAKE) -C GSD_Dictionary/src/ libGSDDict.so

%.o: %.c
	$(GCC) $(CFLAGS) -O2 -c $< $(EXTLIBS) -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	rm -rf include/*.gch
	$(MAKE) -C GSD_Dictionary/src/ clean
	$(MAKE) -C test clean

.PHONY: test
test: libGSDDict.so libGSDGC.so
	$(MAKE) -C test
