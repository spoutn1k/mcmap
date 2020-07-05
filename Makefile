# if you don't want png support, remove "-DWITHPNG", "-lpng" and "draw_png.cpp" below
CC=g++

CFLAGS=-O3 -std=c++17 -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -D_FILE_OFFSET_BITS=64 -fopenmp
LDFLAGS=-lz -lpng -lstdc++fs -fopenmp

DCFLAGS=-g -O0 -std=c++17 -c -Wall -D_DEBUG -DWITHPNG -D_FILE_OFFSET_BITS=64
DLDFLAGS=-lz -lpng -lstdc++fs

PCFLAGS=-O3 -std=c++17 -c -Wall -pg -pedantic -DWITHPNG -D_FILE_OFFSET_BITS=64 -fopenmp
PLDFLAGS=-lz -lpng -lstdc++fs -fopenmp -pg

SOURCES := $(wildcard src/*.cpp) src/include/fmt/format.cpp

OBJECTS=$(SOURCES:.cpp=.default.o)
DOBJECTS=$(SOURCES:.cpp=.debug.o)
POBJECTS=$(SOURCES:.cpp=.profiling.o)

EXECUTABLE=mcmap

all: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

debug: $(DOBJECTS)
	$(CC) $(DOBJECTS) $(DLDFLAGS) -o $(EXECUTABLE)

profile: $(POBJECTS)
	$(CC) $(POBJECTS) $(PLDFLAGS) -o $(EXECUTABLE)

analyse:
	gprof $(EXECUTABLE) gmon.out | less -S

clean:
	find src -name "*.o" -exec rm {} \;

realClean: clean
	rm -f mcmap output.png defaultcolors.txt

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.debug.o: %.cpp
	$(CC) $(DCFLAGS) $< -o $@

%.profiling.o: %.cpp
	$(CC) $(PCFLAGS) $< -o $@
