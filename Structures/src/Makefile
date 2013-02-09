GCC=/usr/bin/gcc
CFLAGS=-g -std=c99 -Wall -O2 -fPIC

libGSDDict.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDDict.so.1 -o libGSDDict.so.1.0 *.o
	ln -sf libGSDDict.so.1.0 libGSDDict.so.1
	ln -sd libGSDDict.so.1.0 libGSDDict.so

*.o:
	$(GCC) $(CFLAGS) -c *.c

clean:
	rm -rf *.o
	rm -rf test*.run
	rm -rf libGSDDict.so*

dottest: libGSDDict.so
	$(GCC) $(CFLAGS) -I. -L. test/dot.c -lGSDDict -otestdot.run
	LD_LIBRARY_PATH="." ./testdot.run > /tmp/testdot.dot
	dot -v -Tx11 /tmp/temp.dot

