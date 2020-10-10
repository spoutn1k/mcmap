CFLAGS=-O3 -c -std=c++17 -Wall -I../src/include
LDFLAGS=-s

ifneq ($(OS), MACOS)
LDFLAGS+=-lstdc++fs
endif

all: json2bson nbt2json regionReader extractChunk memProf

json2bson: ./json2bson.default.o ../src/include/fmt/format.default.o
	$(CXX) $^ $(LDFLAGS) -o $@

nbt2json: ./nbt2json.default.o ../src/include/fmt/format.default.o
	$(CXX) $^ $(LDFLAGS) -lz -o $@

regionReader: ./regionReader.default.o ../src/include/fmt/format.default.o
	$(CXX) $^ $(LDFLAGS) -lz -o $@

extractChunk: ./extractChunk.default.o ../src/include/fmt/format.default.o
	$(CXX) $^ $(LDFLAGS) -lz -o $@

memProf: ./memProf.c
	$(CC) $^ -o $@

%.default.o: %.cpp
	$(CXX) $(CFLAGS) $< -o $@

clean:
	find . -name '*o' -exec rm -fr {} \;

realClean: clean
	git clean -fdx
