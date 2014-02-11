GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC -pthread

libGSDStructures.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDStructures.so.1.0.0 -o libGSDStructures.so.1.0.0 *.o
	ln -sf libGSDStructures.so.1.0.0 libGSDStructures.so.1.0
	ln -sf libGSDStructures.so.1.0.0 libGSDStructures.so.1
	ln -sd libGSDStructures.so.1.0.0 libGSDStructures.so

*.o:
	$(GCC) $(CFLAGS) -O2 -c */*.c

clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.t.run
	rm -rf */*.gch

libDBGGSDStructures.so: clean
	rm -rf *.o
	$(GCC) $(CFLAGS) -g -O0 -DDEV_ASSERTS -c */*.c
	$(GCC) -g -shared -Wl,-soname,libDBGGSDStructures.so.1.0.0 -o libDBGGSDStructures.so.1.0.0 *.o
	ln -sf libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so.1.0
	ln -sf libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so.1
	ln -sd libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so

libGrindGSDStructures.so: clean
	$(GCC) $(CFLAGS) -I/usr/include/valgrind -g -O0 -DDEV_ASSERTS -c */*.c
	$(GCC) -g -shared -Wl,-soname,libGrindGSDStructures.so.1.0.0 -o libGrindGSDStructures.so.1.0.0 *.o
	ln -sf libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so.1.0
	ln -sf libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so.1
	ln -sd libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so

test: clean libDBGGSDStructures.so
	ulimit -c unlimited; \
	for i in */*.t; do \
		echo "\n============\n$$i\n============\n"; \
		$(GCC) $(CFLAGS) -O2 -L../../ -L. -xc $$i -lDBGGSDStructures -lpthread -lGSDPRM -o $$i.run; \
		LD_LIBRARY_PATH=".:../../" ./$$i.run; \
	done
