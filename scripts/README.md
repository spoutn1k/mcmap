# Scripts

This directory contains various scripts to use when debugging and compiling `mcmap`.

- `json2bson` is used to encode the color file before pasting it in the code;
- `nbt2json` takes a NBT file (as found in `level.dat`) and pastes its output as json;
- `regionReader` reads a region file (`.mca` files) and prints all the chunks present in it;
- `extractChunk` extracts a chunk from a given region file;

To print a chunk as json, you can pipe those scripts together:
```
./extractChunk <region file> X Z | ./nbt2json | python -m json.tool
```

## Compilation

Compile using `-DNBT_TOOLS=1` when calling `cmake`.
