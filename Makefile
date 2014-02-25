GCC=/usr/bin/gcc
CFLAGS := -O2 -std=gnu99 -Wall -fPIC -pthread

all: libGSDHashlib.so libGSDGC.so libGSDPRM.so libGSDStructures.so libGSDTokenizer.so

libGSDHashlib.so:
	make -C Hashlib/src libGSDHashlib.so
	ln -s Hashlib/src/*.so* ./

libGSDTokenizer.so:
	make -C Tokenizer/src libGSDTokenizer.so
	ln -s Tokenizer/src/*.so* ./

libGSDGC.so:
	make -C GC/src libGSDGC.so
	ln -s GC/src/*.so* ./

libGSDStructures.so:
	make -C Structures/src libGSDStructures.so
	ln -s Structures/src/*.so* ./

clean:
	make -C Hashlib/src clean
	make -C GC/src clean
	make -C PRM/src clean
	make -C Structures/src clean
	rm -rf *.so*

