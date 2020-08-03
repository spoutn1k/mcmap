CC=g++
SHUSH=--no-print-directory

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

# BCOLORS has to be achieved before the objects, as colors.cpp depends on it
# so those split rules ensure it is made before. SHUSH makes it shut up about
# entering the same directory
all:
	@ $(MAKE) $(SHUSH) $(BCOLORS)
	@ $(MAKE) $(SHUSH) $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

profile:
	@ $(MAKE) $(SHUSH) $(BCOLORS)
	@ $(MAKE) $(SHUSH) $(POBJECTS)
	$(CC) $(POBJECTS) $(PLDFLAGS) -o $(EXECUTABLE)

$(BCOLORS): $(JCOLORS)
	$(MAKE) -C scripts json2bson
	./scripts/json2bson $(JCOLORS) > $@

analyse:
	gprof $(EXECUTABLE) gmon.out | less -S

clean:
	find src -name *o -exec rm {} \;
	$(MAKE) -C scripts $@

realClean: clean
	rm -fr mcmap output.png $(BCOLORS)
	$(MAKE) -C scripts $@

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.profiling.o: %.cpp
	$(CC) $(PCFLAGS) $< -o $@
