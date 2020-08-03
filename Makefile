CC=g++

CFLAGS=-O3 -std=c++17 -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -D_FILE_OFFSET_BITS=64 -fopenmp -Isrc/include
LDFLAGS=-lz -lpng -lstdc++fs -fopenmp

PCFLAGS=-O3 -std=c++17 -c -Wall -pg -pedantic -DWITHPNG -D_FILE_OFFSET_BITS=64 -fopenmp
PLDFLAGS=-lz -lpng -lstdc++fs -fopenmp -pg

SOURCES := $(wildcard src/*.cpp) src/include/fmt/format.cpp

OBJECTS=$(SOURCES:.cpp=.default.o)
DOBJECTS=$(SOURCES:.cpp=.debug.o)
POBJECTS=$(SOURCES:.cpp=.profiling.o)

EXECUTABLE=mcmap

JCOLORS=src/colors.json
BCOLORS=src/colors.bson

all: $(BCOLORS) $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

profile: $(BCOLORS) $(POBJECTS)
	$(CC) $(POBJECTS) $(PLDFLAGS) -o $(EXECUTABLE)

$(BCOLORS): $(JCOLORS)
	make -C scripts json2bson
	./scripts/json2bson $(JCOLORS) > $@

analyse:
	gprof $(EXECUTABLE) gmon.out | less -S

clean:
	find src -name *o -exec rm {} \;
	make -C scripts clean

realClean: clean
	rm -fr mcmap output.png $(BCOLORS)
	make -C scripts realClean

%.default.o: %.cpp $(BCOLORS)
	$(CC) $(CFLAGS) $< -o $@

%.profiling.o: %.cpp
	$(CC) $(PCFLAGS) $< -o $@
