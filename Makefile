ifeq ($(shell env | grep OS),)
	export OS=LINUX
endif

SHUSH=--no-print-directory

CFLAGS=-O3 -std=c++17 -c -Wall -fomit-frame-pointer -pedantic -D_FILE_OFFSET_BITS=64 -Isrc/include
LDFLAGS=-s -lz -lpng

# Resolve OpenMP values
ifeq ($(OS), MACOS)
# Make sure the OpenMP directives are pre-processed by Clang
# Cause g++ is clang that makes sense thanks tim apple
	CFLAGS += -Xpreprocessor -fopenmp
	LDFLAGS += -lomp
else
	# We assume linux with the actual Gnu gcc
	CFLAGS += -fopenmp
	LDFLAGS += -lgomp -lstdc++fs
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
