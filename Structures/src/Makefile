GCC=/usr/bin/gcc
CFLAGS=-std=gnu99 -Wall -fPIC -pthread

libGSDDict.so: *.o
	$(GCC) -shared -Wl,-soname,libGSDDict.so.1 -o libGSDDict.so.1.0 *.o
	ln -sf libGSDDict.so.1.0 libGSDDict.so.1
	ln -sd libGSDDict.so.1.0 libGSDDict.so

*.o:
	$(GCC) $(CFLAGS) -O2 -c *.c

clean:
	rm -rf *.o
	rm -rf *.so*
	rm -rf *.run

libDBGGSDDict.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -g -O0 -c *.c
	$(GCC) -g -shared -Wl,-soname,libDBGGSDDict.so.1 -o libDBGGSDDict.so.1.0 *.o
	ln -sf libDBGGSDDict.so.1.0 libDBGGSDDict.so.1
	ln -sd libDBGGSDDict.so.1.0 libDBGGSDDict.so

libGrindGSDDict.so:
	rm -rf *.o
	$(GCC) $(CFLAGS) -I/usr/include/valgrind -g -O0 -c *.c
	$(GCC) -g -shared -Wl,-soname,libGrindGSDDict.so.1 -o libGrindGSDDict.so.1.0 *.o
	ln -sf libGrindGSDDict.so.1.0 libGrindGSDDict.so.1
	ln -sd libGrindGSDDict.so.1.0 libGrindGSDDict.so

grinddottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L. test/dottest.c -lGrindGSDDict -otestdot.run
	LD_LIBRARY_PATH="." valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
						./testdot.run > /dev/null 2>grind.txt

gdbdottest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L. test/dottest.c -lGrindGSDDict -otestdot.run
	LD_LIBRARY_PATH="." gdb ./testdot.run

dottest: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L. test/dottest.c -lGSDDict -otestdot.run
	LD_LIBRARY_PATH="." ./testdot.run > /tmp/testdot.dot
	dot -Tx11 /tmp/testdot.dot

stresstest: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L. test/stress.c -lGrindGSDDict -lpthread -oteststress.run
	LD_LIBRARY_PATH="." ./teststress.run
#	LD_LIBRARY_PATH="." gdb ./teststress.run
#	LD_LIBRARY_PATH="." valgrind --leak-check=full \
#								 --read-var-info=yes \
#								 --track-origins=yes \
#								 --show-reachable=yes \
#						./teststress.run 2> grind.txt
#	LD_LIBRARY_PATH="." valgrind --tool=helgrind \
#								 --read-var-info=yes \
#								 --history-level=full \
#						./teststress.run 2> grind.txt

performance: libGSDDict.so
	$(GCC) $(CFLAGS) -O2 -I. -L. test/performance.c -lGSDDict -lpthread -lrt -operformance.run
	LD_LIBRARY_PATH="." ./performance.run

grind_performance: libGrindGSDDict.so
	$(GCC) $(CFLAGS) -O0 -I. -L. test/performance.c -lGrindGSDDict -lpthread -lrt -operformance.run
	LD_LIBRARY_PATH="." valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./performance.run 2> grind.txt

debug_performance: libDBGGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L. test/performance.c -lDBGGSDDict -lpthread -lrt -operformance.run
	LD_LIBRARY_PATH="." gdb ./performance.run

debug_apitest: libDBGGSDDict.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L. test/apitest.c -lDBGGSDDict -lpthread -oapitest.run
	LD_LIBRARY_PATH="." gdb ./apitest.run

