# if you don't want png support, remove "-DWITHPNG", "-lpng" and "draw_png.cpp" below
CC=g++
CFLAGS=-O2 -c -Wall -fomit-frame-pointer -pedantic -DWITHPNG
LDFLAGS=-O2 -lz -lpng -fomit-frame-pointer
DCFLAGS=-g -c -Wall -D_DEBUG -DWITHPNG
DLDFLAGS=-g -lz -lpng
SOURCES=main.cpp helper.cpp nbt.cpp draw.cpp colors.cpp worldloader.cpp filesystem.cpp globals.cpp draw_png.cpp
OBJECTS=$(SOURCES:.cpp=.default.o)
OBJECTS_TURBO=$(SOURCES:.cpp=.turbo.o)
DOBJECTS=$(SOURCES:.cpp=.debug.o)
OBJECTS64=$(SOURCES:.cpp=.64.o)
EXECUTABLE=mcmap

# default, zlib shared
all: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

# use this one on windows so you don't have to supply zlib1.dll
static: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -static -o $(EXECUTABLE)

# crosscompile static version for 64bit (using this one on windows)
x64: $(OBJECTS64)
	x86_64-w64-mingw32-g++ $(OBJECTS64) $(LDFLAGS) -m64 -static -o $(EXECUTABLE)

# debug, zlib shared
debug: $(DOBJECTS)
	$(CC) $(DOBJECTS) $(DLDFLAGS) -static -o $(EXECUTABLE)

# use this to build a binary optimized for your cpu - not recommended for distribution
turbo: $(OBJECTS_TURBO)
	$(CC) $(OBJECTS_TURBO) $(LDFLAGS) -march=native -mtune=native -o $(EXECUTABLE)

clean:
	rm *.o

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

%.turbo.o: %.cpp
	$(CC) $(CFLAGS) -march=native -mtune=native $< -o $@

%.debug.o: %.cpp
	$(CC) $(DCFLAGS) $< -o $@

%.64.o: %.cpp
	x86_64-w64-mingw32-g++ $(CFLAGS) -m64 $< -o $@