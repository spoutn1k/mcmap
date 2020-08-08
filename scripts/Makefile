CC=g++

CFLAGS=-O3 -c -std=c++17 -Wall -I../src/include
LDFLAGS=-lstdc++fs

all: json2bson nbt2json

json2bson: ./json2bson.default.o ../src/include/fmt/format.default.o
	$(CC) $^ $(LDFLAGS) -o $@

nbt2json: ./nbt2json.default.o ../src/include/fmt/format.default.o
	$(CC) $^ $(LDFLAGS) -lz -o $@

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	find . -name '*o' -exec rm -fr {} \;

realClean: clean
	rm -fr json2bson nbt2json
