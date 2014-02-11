gdbdottest: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/dottest.c -lGrindGSDStructures -lGSDPRM -DDEV_ASSERTS -otestdot.run
	LD_LIBRARY_PATH=".:../../" gdb ./testdot.run

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

stringtest: libGSDStructures.so
	$(GCC) $(CFLAGS) -O2 -I. -L../../ -L. test/stringdict.c -lGSDStructures -lGSDPRM -lGSDHashlib -lpthread -ostringdict.run
	LD_LIBRARY_PATH=".:../../" ./stringdict.run

grind_stringtest: libGSDStructures.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/stringdict.c -lGSDStructures -lGSDPRM -lGSDHashlib -lpthread -ostringdict.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./stringdict.run 2> grind.txt

debug_stringtest: libGSDStructures.so
	$(GCC) $(CFLAGS) -g -O0 -I. -L../../ -L. test/stringdict.c -lGSDStructures -lGSDPRM -lGSDHashlib -lpthread -ostringdict.run
	LD_LIBRARY_PATH=".:../../" gdb ./stringdict.run


grind_apitest: libGrindGSDStructures.so
	$(GCC) $(CFLAGS) -O0 -g -I. -L../../ -L. test/apitest.c -lGrindGSDStructures -lGSDPRM -lpthread -DDEV_ASSERTS -oapitest.run
	LD_LIBRARY_PATH=".:../../" valgrind --leak-check=full \
								 --read-var-info=yes \
								 --track-origins=yes \
								 --show-reachable=yes \
						./apitest.run 2> grind.txt


