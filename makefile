CC=mpicc
CFLAGS=-fPIC -g -O0
LFLAGS=

.PHONY: directories

all: directories build/lib/libcmpi.so build/bin/load_and_scatter

build/lib/libcmpi.so: build/permute.o build/utils.o build/hash.o build/load.o
	$(CC) -shared $(CFLAGS) $(LFLAGS) -o build/lib/libcmpi.so build/permute.o build/utils.o build/load.o build/hash.o 

build/permute.o: src/permute.c src/permute.h src/utils.h
	$(CC) -c $(CFLAGS) -o build/permute.o src/permute.c

build/utils.o: src/utils.h
	$(CC) -c $(CFLAGS) -o build/utils.o src/utils.c

build/hash.o: src/hash.c src/hash.h
	$(CC) -c $(CFLAGS) -o build/hash.o src/hash.c

build/load.o: src/load.c src/load.h
	$(CC) -c $(CFLAGS) -o build/load.o src/load.c

build/bin/load_and_scatter: examples/load_and_scatter.c build/lib/libcmpi.so
	$(CC) $(CFLAGS) $(LFLAGS) -Lbuild/lib -Wl,-rpath=build/lib -o build/bin/load_and_scatter examples/load_and_scatter.c -lcmpi -lm

clean:
	rm -rf build

directories: build/lib build/bin

build/lib:
	mkdir -p build/lib

build/bin:
	mkdir -p build/bin
