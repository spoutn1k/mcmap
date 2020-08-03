CC=g++

CFLAGS=-O3 -c -std=c++17 -Wall -I../src/include

json2bson: ./json2bson.default.o ../src/include/fmt/format.default.o
	$(CC) $^ -o $@

%.default.o: %.cpp
	$(CC) $(CFLAGS) $< -o $@

clean:
	find . -name *o -exec rm {} \;

realClean:
	rm -fr json2bson
