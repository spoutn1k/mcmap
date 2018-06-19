# mcmap - Tiny map visualizer

![sample](http://skutnik.iiens.net/sample_output.png)

mcmap is a flexible tool allowing you to create isometric renders of your Minecraft save file. Originally designed to be used on servers, it works via command-line, but can be used on Windows with external front end interfaces.

## Usage

__Linux / MacOS__
```
./mcmap <options> ~/.minecraft/saves/<your save>
```

__Windows__
```
mcmap %APPDATA%\.minecraft\saves\<your save>
```

## Options

| Name         | Description                              |
|--------------|------------------------------------------|
|-from X Z     |sets the coordinates of the chunk to start rendering at|
|-to X Z       |sets the coordinates of the chunk to end rendering at|
|-cave         |renders a map of all explored caves|
|-blendcave    |overlay caves over normal map; doesn't work with incremental rendering (some parts will be hidden)|
|-night        |renders the world at night using blocklight (torches)|
|-skylight     |use skylight when rendering map (shadows below trees etc.) Using this with -night makes a difference|
|-noise VAL    |adds some noise to certain blocks, reasonable values are 0-20|
|-height VAL   |maximum height at which blocks will be rendered|
|-min/max VAL  |minimum/maximum Y index (height) of blocks to render|
|-file NAME    |sets the output filename to 'NAME'; default is output.png|
|-mem VAL      |sets the amount of memory (in MiB) used for rendering. mcmap will use incremental rendering or disk caching to stick to this limit. Default is 1800.|
|-colors NAME  |loads user defined colors from file 'NAME'|
|-dumpcolors   |creates a file which contains the default colors being used for rendering. Can be used to modify them and then loaded with -colors|
|-north -east -south -west |controls which direction will point to the *top left* corner it only makes sense to pass one of them; East is default|
|-blendall     |always use blending mode for blocks|
|-hell         |render the hell/nether dimension of the given world|
|-end          |render the end dimension of the given world|
|-serverhell   |force cropping of blocks at the top (use for nether servers)|
|-nowater      |render map with water being clear (as if it were air)|
|-texture NAME |extract colors from png file 'NAME'; eg. terrain.png|
|-biomes       |apply biome colors to grass/leaves; requires that you run Donkey Kong's biome extractor first on your world|
|-biomecolors PATH  |load grasscolor.png and foliagecolor.png from 'PATH'|
|-info NAME    |Write information about map to file 'NAME' You can choose the format by using file extensions .xml, .json or .txt (default)|
|-split PATH   |create tiled output (128x128 to 4096x4096) in given PATH|
|-marker c x z |place marker at x z with color c (r g b w)|

*Note: Currently you need both -from and -to to define bounds, otherwise the entire world will be rendered.*

## Install

__On Linux/Unix__

```
git clone https://github.com/jbman100/mcmap
cd mcmap
make
```

From then on, the binary will be in the mcmap directory, it's yours to move in your PATH.
