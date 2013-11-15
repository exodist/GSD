GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC -pthread

libGSDStructures.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDStructures.so.1.0.0 -o libGSDStructures.so.1.0.0 *.o
	ln -sf libGSDStructures.so.1.0.0 libGSDStructures.so.1.0
	ln -sf libGSDStructures.so.1.0.0 libGSDStructures.so.1
	ln -sd libGSDStructures.so.1.0.0 libGSDStructures.so

*.o:
	$(GCC) $(CFLAGS) -g -O1 -c *.c

clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.run

libDBGGSDStructures.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -g -O0 -DDEV_ASSERTS -c *.c
	$(GCC) -g -shared -Wl,-soname,libDBGGSDStructures.so.1.0.0 -o libDBGGSDStructures.so.1.0.0 *.o
	ln -sf libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so.1.0
	ln -sf libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so.1
	ln -sd libDBGGSDStructures.so.1.0.0 libDBGGSDStructures.so

libGrindGSDStructures.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -I/usr/include/valgrind -g -O0 -DDEV_ASSERTS -c *.c
	$(GCC) -g -shared -Wl,-soname,libGrindGSDStructures.so.1.0.0 -o libGrindGSDStructures.so.1.0.0 *.o
	ln -sf libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so.1.0
	ln -sf libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so.1
	ln -sd libGrindGSDStructures.so.1.0.0 libGrindGSDStructures.so

grinddottest: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/dottest.c -lGrindGSDStructures -lGSDPRM -DDEV_ASSERTS -otestdot.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
						./testdot.run > /dev/null 2>grind.txt

gdbdottest: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/dottest.c -lGrindGSDStructures -lGSDPRM -DDEV_ASSERTS -otestdot.run
	LD_LIBRARY_PATH=".:../../" gdb ./testdot.run

dottest: libGSDStructures.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/dottest.c -lGSDStructures -lGSDPRM -otestdot.run
	LD_LIBRARY_PATH=".:../../" ./testdot.run > /tmp/testdot.dot
	dot -Tx11 /tmp/testdot.dot

debug_stress: libDBGGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/stress.c -lDBGGSDStructures -lGSDPRM -lpthread -DDEV_ASSERTS -ostress.run
	LD_LIBRARY_PATH=".:../../" gdb ./stress.run

grind_stress: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -I. -L../../ -L. test/stress.c -lGrindGSDStructures -lGSDPRM -lpthread -DDEV_ASSERTS -ostress.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./stress.run 2> grind.txt
#	LD_LIBRARY_PATH=".:../../" valgrind --tool=helgrind \
#								 --read-var-info=yes \
#								 --history-level=full \
#						./teststress.run 2> grind.txt

stress: libGSDStructures.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/stress.c -lGSDStructures -lGSDPRM -lpthread -ostress.run
	LD_LIBRARY_PATH=".:../../" ./stress.run

performance: libGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/performance.c -lGSDStructures -lGSDPRM -lpthread -lrt -operformance.run
	LD_LIBRARY_PATH=".:../../" ./performance.run

grind_performance: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -I. -L../../ -L. test/performance.c -lGrindGSDStructures -lGSDPRM -lpthread -lrt -DDEV_ASSERTS -operformance.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./performance.run 2> grind.txt

debug_performance: libDBGGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/performance.c -lDBGGSDStructures -lGSDPRM -lpthread -lrt -DDEV_ASSERTS -operformance.run
	LD_LIBRARY_PATH=".:../../" gdb ./performance.run

debug_apitest: libDBGGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. -L../../ test/apitest.c -lDBGGSDStructures -lGSDPRM -lpthread -DDEV_ASSERTS -oapitest.run
	LD_LIBRARY_PATH=".:../../" gdb ./apitest.run

apitest: libGSDStructures.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. -L../../ test/apitest.c -lGSDStructures -lGSDPRM -lpthread -oapitest.run
	LD_LIBRARY_PATH=".:../../" ./apitest.run

memtest: libGSDStructures.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/memory.c -lGSDStructures -lGSDPRM -lpthread -omemorytest.run
	LD_LIBRARY_PATH=".:../../" ./memorytest.run

grind_apitest: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/apitest.c -lGrindGSDStructures -lGSDPRM -lpthread -DDEV_ASSERTS -oapitest.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./apitest.run 2> grind.txt


