# if you don't want png support, remove "-DWITHPNG", "-lpng" and "draw_png.cpp" below
CC=g++
CFLAGS=-O3 -msse -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/local/include
LDFLAGS=-O3 -msse -lz -lpng -fomit-frame-pointer -L/usr/local/lib
DCFLAGS=-g -O0 -c -Wall -D_DEBUG -DWITHPNG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/local/include
DLDFLAGS=-g -O0 -lz -lpng -L/usr/local/lib
SOURCES=main.cpp helper.cpp nbt.cpp colors.cpp worldloader.cpp filesystem.cpp globals.cpp draw_png.cpp extractcolors.cpp pngreader.cpp block.cpp
OBJECTS=$(SOURCES:.cpp=.default.o)
OBJECTS_TURBO=$(SOURCES:.cpp=.turbo.o)
DOBJECTS=$(SOURCES:.cpp=.debug.o)
OBJECTS64=$(SOURCES:.cpp=.64.o)
EXECUTABLE=mcmap
EXECUTABLE64=mcmap64

CFLAGSX11=-O3 -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -I/usr/local/include -I/usr/X11/include
LDFLAGSX11=-O3 -lz -lpng -fomit-frame-pointer -L/usr/local/lib -L/usr/X11/lib

# default, zlib and libpng shared
all: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

# fixing for X11 dependency
x11: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGSX11) -o $(EXECUTABLE)

# use this one on windows so you don't have to supply zlib1.dll
static: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -static -o $(EXECUTABLE)

# crosscompile static version for win 64bit (using this one on windows)
x64: $(OBJECTS64)
	x86_64-w64-mingw32-g++ $(OBJECTS64) $(LDFLAGS) -m64 -static -o $(EXECUTABLE64)

# debug, zlib shared
debug: $(DOBJECTS)
	$(CC) $(DOBJECTS) $(DLDFLAGS) -static -o $(EXECUTABLE)

# use this to build a binary optimized for your cpu - not recommended for distribution
turbo: $(OBJECTS_TURBO)
	$(CC) $(OBJECTS_TURBO) $(LDFLAGS) -march=native -mtune=native -o $(EXECUTABLE)

clean:
	rm -f *.o

realClean: clean
	rm -f mcmap output.png defaultcolors.txt

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.turbo.o: %.cpp
	$(CC) $(CFLAGS) -march=native -mtune=native $< -o $@

%.debug.o: %.cpp
	$(CC) $(DCFLAGS) $< -o $@

%.64.o: %.cpp
	x86_64-w64-mingw32-g++ $(CFLAGS) -m64 $< -o $@
