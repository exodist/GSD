GCC=/usr/bin/gcc
CFLAGS=-std=c99 -Wall -fPIC

libGSDDict.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDDict.so.1 -o libGSDDict.so.1.0 *.o
	ln -sf libGSDDict.so.1.0 libGSDDict.so.1
	ln -sd libGSDDict.so.1.0 libGSDDict.so

*.o:
	$(GCC) $(CFLAGS) -O2 -c *.c

clean:
	rm -rf *.o
	rm -rf test*.run
	rm -rf libGSDDict.so*
	rm -rf libGrindGSDDict.so*

libGrindGSDDict.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -g -O0 -c *.c
	$(GCC) -g -shared -Wl,-soname,libGrindGSDDict.so.1 -o libGrindGSDDict.so.1.0 *.o
	ln -sf libGrindGSDDict.so.1.0 libGrindGSDDict.so.1
	ln -sd libGrindGSDDict.so.1.0 libGrindGSDDict.so

grinddottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L. test/dot.c -lGrindGSDDict -otestdot.run
	LD_LIBRARY_PATH="." valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
						./testdot.run > /dev/null 2>grind.txt

gdbdottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L. test/dot.c -lGrindGSDDict -otestdot.run
	LD_LIBRARY_PATH="." gdb ./testdot.run

dottest: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L. test/dot.c -lGSDDict -otestdot.run
	LD_LIBRARY_PATH="." ./testdot.run > /tmp/testdot.dot
	dot -Tx11 /tmp/testdot.dot

