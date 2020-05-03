# if you don't want png support, remove "-DWITHPNG", "-lpng" and "draw_png.cpp" below
CC=g++

CFLAGS=-O3 -std=c++17 -msse -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -I/usr/local/include
LDFLAGS=-O3 -msse -lz -lpng -fomit-frame-pointer -L/usr/local/lib

DCFLAGS=-g -O0 -std=c++17 -c -Wall -D_DEBUG -DWITHPNG -I/usr/local/include
DLDFLAGS=-msse -lz -lpng -L/usr/local/lib

SOURCES=main.cpp helper.cpp nbt.cpp colors.cpp worldloader.cpp filesystem.cpp globals.cpp draw_png.cpp block.cpp settings.cpp
OBJECTS=$(SOURCES:.cpp=.default.o)
DOBJECTS=$(SOURCES:.cpp=.debug.o)

EXECUTABLE=mcmap

CFLAGSX11=-O3 -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG -I/usr/local/include -I/usr/X11/include
LDFLAGSX11=-O3 -lz -lpng -fomit-frame-pointer -L/usr/local/lib -L/usr/X11/lib

# default, zlib and libpng shared
all: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

# fixing for X11 dependency
x11: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGSX11) -o $(EXECUTABLE)

# debug, zlib shared
debug: $(DOBJECTS)
	$(CC) $(DOBJECTS) $(DLDFLAGS) -o $(EXECUTABLE)
	#$(CC) $(DOBJECTS) $(DLDFLAGS) -static -o $(EXECUTABLE)

clean:
	rm -f *.o *gch

realClean: clean
	rm -f mcmap output.png defaultcolors.txt

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.debug.o: %.cpp
	$(CC) $(DCFLAGS) $< -o $@
