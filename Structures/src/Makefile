GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC -pthread

libGSDDict.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDDict.so.1.0.0 -o libGSDDict.so.1.0.0 *.o
	ln -sf libGSDDict.so.1.0.0 libGSDDict.so.1.0
	ln -sf libGSDDict.so.1.0.0 libGSDDict.so.1
	ln -sd libGSDDict.so.1.0.0 libGSDDict.so

*.o:
	$(GCC) $(CFLAGS) -O2 -c *.c

clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.run

libDBGGSDDict.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -g -O0 -DDEV_ASSERTS -c *.c
	$(GCC) -g -shared -Wl,-soname,libDBGGSDDict.so.1.0.0 -o libDBGGSDDict.so.1.0.0 *.o
	ln -sf libDBGGSDDict.so.1.0.0 libDBGGSDDict.so.1.0
	ln -sf libDBGGSDDict.so.1.0.0 libDBGGSDDict.so.1
	ln -sd libDBGGSDDict.so.1.0.0 libDBGGSDDict.so

libGrindGSDDict.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -I/usr/include/valgrind -g -O0 -DDEV_ASSERTS -c *.c
	$(GCC) -g -shared -Wl,-soname,libGrindGSDDict.so.1.0.0 -o libGrindGSDDict.so.1.0.0 *.o
	ln -sf libGrindGSDDict.so.1.0.0 libGrindGSDDict.so.1.0
	ln -sf libGrindGSDDict.so.1.0.0 libGrindGSDDict.so.1
	ln -sd libGrindGSDDict.so.1.0.0 libGrindGSDDict.so

grinddottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/dottest.c -lGrindGSDDict -lGSDPRM -DDEV_ASSERTS -otestdot.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
						./testdot.run > /dev/null 2>grind.txt

gdbdottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/dottest.c -lGrindGSDDict -lGSDPRM -DDEV_ASSERTS -otestdot.run
	LD_LIBRARY_PATH=".:../../" gdb ./testdot.run

dottest: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/dottest.c -lGSDDict -lGSDPRM -otestdot.run
	LD_LIBRARY_PATH=".:../../" ./testdot.run > /tmp/testdot.dot
	dot -Tx11 /tmp/testdot.dot

debug_stress: libDBGGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/stress.c -lDBGGSDDict -lGSDPRM -lpthread -DDEV_ASSERTS -ostress.run
	LD_LIBRARY_PATH=".:../../" gdb ./stress.run

grind_stress: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -O0 -I. -L../../ -L. test/stress.c -lGrindGSDDict -lGSDPRM -lpthread -DDEV_ASSERTS -ostress.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./stress.run 2> grind.txt
#	LD_LIBRARY_PATH=".:../../" valgrind --tool=helgrind \
#								 --read-var-info=yes \
#								 --history-level=full \
#						./teststress.run 2> grind.txt

stress: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/stress.c -lGSDDict -lGSDPRM -lpthread -ostress.run
	LD_LIBRARY_PATH=".:../../" ./stress.run

performance: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/performance.c -lGSDDict -lGSDPRM -lpthread -lrt -operformance.run
	LD_LIBRARY_PATH=".:../../" ./performance.run

grind_performance: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -O0 -I. -L../../ -L. test/performance.c -lGrindGSDDict -lGSDPRM -lpthread -lrt -DDEV_ASSERTS -operformance.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./performance.run 2> grind.txt

debug_performance: libDBGGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/performance.c -lDBGGSDDict -lGSDPRM -lpthread -lrt -DDEV_ASSERTS -operformance.run
	LD_LIBRARY_PATH=".:../../" gdb ./performance.run

debug_apitest: libDBGGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. -L../../ test/apitest.c -lDBGGSDDict -lGSDPRM -lpthread -DDEV_ASSERTS -oapitest.run
	LD_LIBRARY_PATH=".:../../" gdb ./apitest.run

apitest: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. -L../../ test/apitest.c -lGSDDict -lGSDPRM -lpthread -oapitest.run
	LD_LIBRARY_PATH=".:../../" ./apitest.run

memtest: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/memory.c -lGSDDict -lGSDPRM -lpthread -omemorytest.run
	LD_LIBRARY_PATH=".:../../" ./memorytest.run

grind_apitest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/apitest.c -lGrindGSDDict -lGSDPRM -lpthread -DDEV_ASSERTS -oapitest.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./apitest.run 2> grind.txt


