GCC=/usr/bin/gcc
CFLAGS := -O2 -std=gnu99 -Wall -fPIC -pthread

libGSDTokenizer.so: $(shell ls *.c | sed 's,\.c,.o,g')
	$(GCC) -shared -Wl,-soname,libGSDTokenizer.so.1.0.0 -o libGSDTokenizer.so.1.0.0 *.o $(EXTLIBS)
	ln -sf libGSDTokenizer.so.1.0.0 libGSDTokenizer.so.1.0
	ln -sf libGSDTokenizer.so.1.0.0 libGSDTokenizer.so.1
	ln -sf libGSDTokenizer.so.1.0.0 libGSDTokenizer.so

%.o: %.c
	$(GCC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.gch
	rm -rf include/*.gch
	$(MAKE) -C test clean || true
