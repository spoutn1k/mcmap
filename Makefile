SHUSH=--no-print-directory

CFLAGS=-O3 -std=c++17 -c -Wall -fomit-frame-pointer -pedantic -D_FILE_OFFSET_BITS=64 -Isrc/include
LDFLAGS=-s -lz -lpng -lomp

ifeq ($(OS), MACOS)
# Make sure the openMP directives are pre-processed by Clang
CFLAGS+=-Xpreprocessor -fopenmp
else
# We assume linux
CFLAGS+=-fopenmp
LDFLAGS+=-lstdc++fs
endif

SOURCES := $(wildcard src/*.cpp) src/include/fmt/format.cpp
OBJECTS=$(SOURCES:.cpp=.default.o)

EXECUTABLE=mcmap

JCOLORS=src/colors.json
BCOLORS=src/colors.bson

# BCOLORS has to be achieved before the objects, as colors.cpp depends on it
# so those split rules ensure it is made before. SHUSH makes it shut up about
# entering the same directory
all:
	@ $(MAKE) $(SHUSH) $(BCOLORS)
	@ $(MAKE) $(SHUSH) $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

$(BCOLORS): $(JCOLORS)
	$(MAKE) -C scripts json2bson
	./scripts/json2bson $(JCOLORS) > $@

clean:
	find src -name *o -exec rm {} \;
	$(MAKE) -C scripts $@

realClean: clean
	rm -fr mcmap output.png $(BCOLORS)
	$(MAKE) -C scripts $@

%.default.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@
