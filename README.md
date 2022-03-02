# `mcmap` - Isometric map visualizer

![](https://img.shields.io/badge/version-1.16.5-success) ![](https://img.shields.io/badge/version-1.17-success) ![](https://img.shields.io/badge/version-1.18.1-success)

*Original project by Simon Rettberg. All the credit goes to him for the idea and vision.*

`mcmap` is a tool allowing you to create isometric renders of your Minecraft save file.

![sample](assets/sample.png)

This project is under __heavy__ development, but compatible with newer versions of Minecraft.

## Usage

### Basic invocation

```
mcmap <options> path/to/<your save>
```

The standard save path is different between OSes:
- On Linux, it is `$HOME/.minecraft/saves`;
- On macOS, under `~/Library/Application\ Support/minecraft/saves`;
- On Windows, the standard path is `%AppData%\.minecraft\saves`. If using [Ubuntu on Windows](https://ubuntu.com/tutorials/tutorial-ubuntu-on-windows#1-overview), the path to access the save folder is the following: `/mnt/c/<Your user>/AppData/Roaming/.minecraft/saves`.

Native Windows is now supported.
Pre-compiled binaries can be downloaded from the [releases page](https://github.com/spoutn1k/mcmap/releases).
For now, the program can only be used via `terminal`/`powershell` on Linux/Macos or Windows respectively.

An experimental GUI is available for Windows and can be downloaded [here](https://github.com/spoutn1k/mcmap/discussions/63).

### Options

| Name                         | Description                                                                                              |
|------------------------------|----------------------------------------------------------------------------------------------------------|
|`-from X Z`                   |sets the coordinates of the block to start rendering at                                                   |
|`-to X Z`                     |sets the coordinates of the block to end rendering at                                                     |
|`-center X Z`                 |sets the center of a circular render                                                                      |
|`-radius VAL`                 |sets the radius of a circular render                                                                      |
|`-min/max VAL`                |minimum/maximum Y index (height) of blocks to render                                                      |
|`-file NAME`                  |sets the output filename to 'NAME'; default is `./output.png`                                             |
|`-colors NAME`                |sets the custom color file to 'NAME'                                                                      |
|`-nw` `-ne` `-se` `-sw`       |controls which direction will point to the top corner; North-West is default                              |
|`-marker x z color`           |draw a marker at `x` `z` of color `color` in `red`,`green`,`blue` or `white`; can be used up to 256 times |
|`-nowater`                    |do not render water                                                                                       |
|`-nobeacons`                  |do not render beacon beams                                                                                |
|`-shading`                    |toggle shading (brightens blocks depending on height)                                                     |
|`-lighting`                   |toggle lighting (brightens blocks depending on light)                                                     |
|`-nether`                     |render the nether                                                                                         |
|`-end`                        |render the end                                                                                            |
|`-dim[ension] [namespace:]id` |render a dimension by namespaced ID                                                                       |
|`-mb VAL`                     |maximum memory to use at once (default 3.5G, increase for large maps if you have the ram)                 |
|`-fragment VAL`               |render terrain in regions of the specified size (default 1024x1024 blocks)                                |
|`-tile VAL`                   |generate split output in square tiles of the specified size (in pixels) (default 0, disabled)             |
|`-padding`                    |padding around the final image, in pixels (default: 5)                                                    |
|`-h[elp]`                     |display an option summary                                                                                 |
|`-v[erbose]`                  |toggle debug mode                                                                                         |
|`-dumpcolors`                 |dump a json with all defined colors                                                                       |

*Note: Currently you need both `-from` and `-to` OR `-center` and `-radius` to define bounds.*

#### Tips

`mcmap` will render the terrain in batches using all the threads of your computer. Unfortunately, when those batches merge, some artifacts are created: lines appear in oceans where the merge was operated.

Use `-fragment` with a bigger value to limit the amount of batches and thus of artifacts. This is limited by the available memory, as rendering a whole map iin one go may require 10+ gigabytes of ram.

Use `-fragment` with a lower value to increase performance. Fragments of 256x256 and 512x512 blocks are really efficient.

## Color file format

`mcmap` supports changing the colors of blocks. To do so, prepare a custom color file by editing the output of `mcmap -dumpcolors`, and pass it as an argument using the `-colors` argument.

The accepted format is a `json` file, with a specific structure.
The root contains a list of [block IDs](https://minecraft.gamepedia.com/Java_Edition_data_values#Blocks) to modify, with the namespace prefix, such as `namespace:block`.

#### Simple block

To define a color for a simple, regular block, provide an entry in a JSON file.
The color format is a [hexadecimal color code](https://htmlcolorcodes.com/).
If the alpha is not specified, it is assumed to be opaque.

```
"namespace:block": #rrggbbaa (or #rrggbb)
```

Examples:
```
{
    "minecraft:dirt":   #7b573b,
    "minecraft:ice":    #7dadff9f, 
    ...
}
```

#### Complex block

Some blocks are better looking when drawn in a specific way.
To specify that a block has to be drawn differently, you have to provide a `json` structure with the following fields:

```
"namespace:block": {
    "type":     <BlockType>,
    "color":    "#rrggbbaa",
    "accent":   "#rrggbbaa" (Optional)
}
```

The available available block types are:

|Name|Appearance|Accent support|
|-|-|-|
|`Full`|Default. Full-block.|No|
|`Hide`|Do not render the block entirely.|No|
|`Clear`|This block is optimized for transparent block in large quantities, such as glass and water. The top of the block is not rendered, making for a smooth surface when blending blocks together.|No|
|`Thin`|Will color only the top of the block underneath. Used for snow, rails, pressure plates.|No|
|`Slab`|Half block.|No|
|`Stair`|Renders a stair block.|No|
|`Rod`|A slimmer block, used for fences and walls.|No|
|`Wire`|Small dot on the floor, used for tripwire and redstone.|No|
|`Head`|Smaller block, also used for pots, pickles, and mushrooms.|No|
|`Plant`|Used in a variety of cases, renders a leaf-like block.|No|
|`UnderwaterPlant`|Same as `Plant`, but the air is water-colored. Used for sea-grass and kelp.|No|
|`Fire`|Fire-like. Used for fire.|No|
|`Beam`|Internal block type, used for markers and beacon beams.|No|
|`Torch`|Three pixels in a vertical line, with the top pixel rendered with the accent color.|Yes|
|`Ore`|Block with veins of color. The vein is rendered with the accent color.|Yes|
|`Grown`|Blocks that have a different layer on top. Grass, nylium, etc. The top layer is rendered with the accent color.|Yes|
|`Log`|Directionnal block, to render logs/pillars as close as possible. The center of the pillar is rendered with the accent color. Used for logs, pillars, basalt.|Yes|
|`Lamp`|Conditionnal block, to render redstone lamps. If lit, rendered with accent color.|Yes|

__NOTE__: Waterlogged blocks will be rendered within water instead of air by default according to their blockstates. However, sea-grass and kelp are hardcoded to be underwater and their blockstates won't reflect this, so they have to be defined as `UnderwaterPlants`.

Examples:

```
{
    "minecraft:dirt":   "#7b573b",  // Full block with solid color

    "minecraft:grass_block": {
        "type":     "Grown",        // Use a special block type
        "accent":   "#4c7a40",      // Accent supported for `Grown`
        "color":    "#7b573b"
    },

    "minecraft:water": {
        "type": "Clear"
        "color": "#0734c832",       // Transparency enabled
    },
}
```

## Tiled output

Using the `-tile` options with a non-zero value triggers the split output. A folder will be created with the following format:
```
$ ls output
0    104  15  21  28  34  40  47  53  6   66  72  79  85  91  98
1    105  16  22  29  35  41  48  54  60  67  73  8   86  92  99
10   106  17  23  3   36  42  49  55  61  68  74  80  87  93  mapinfo.json
100  11   18  24  30  37  43  5   56  62  69  75  81  88  94
101  12   19  25  31  38  44  50  57  63  7   76  82  89  95
102  13   2   26  32  39  45  51  58  64  70  77  83  9   96
103  14   20  27  33  4   46  52  59  65  71  78  84  90  97
```

To view the generated map, open the HTML file in `contrib/leaflet/index.html`. A file dialog will be present; give it the above `mapinfo.json` to load the map.

## Compilation

`mcmap` depends on the `PNG` and `zlib` libraries.
Development was made using `gcc` version 10, and can be compiled with `gcc` 8 or later or `clang` 10 or later.
Configuration is done using `CMake`.

#### Linux

Getting the libraries depends on your distribution:

- Ubuntu: `apt update && apt install git make g++ libpng-dev cmake`;
- Archlinux: `pacman -S --needed git gcc make libpng cmake`.

Then get the code and compile:
```
git clone https://github.com/spoutn1k/mcmap
mkdir -p mcmap/build && cd mcmap/build
cmake ..
make -j
```

#### macOS

In an Apple environment, you need a developer toolkit recent enough, with the version of `g++ --version` superior to 10. 

Using [`brew`](https://brew.sh/):
```
brew install libpng libomp
git clone https://github.com/spoutn1k/mcmap
mkdir -p mcmap/build && cd mcmap/build
cmake ..
make -j
```

#### Windows

`mcmap` was successfully compiled for Windows using Visual Studio/Visual C++ and MinGW.
As there is no package manager on Windows, [`libpng`](http://www.libpng.org/pub/png/libpng.html) and [`zlib`](https://zlib.net/) need to be compiled/installed manually.
If compiling the GUI version, you will also need [`Qt`](https://www.qt.io/download).

You can also download and set up [Ubuntu on windows](https://ubuntu.com/tutorials/tutorial-ubuntu-on-windows#1-overview) then the steps are the same as Linux/Ubuntu.

### Snapshot support

Snapshots are unsupported by default. One can enable support by using the `SNAPSHOT` `CMake` option. In linux/macOS:
```
git clone https://github.com/spoutn1k/mcmap
mkdir -p mcmap/build && cd mcmap/build
cmake .. -DSNAPSHOT=1
make -j
```

On Windows, this will depend on the software you are using to compile `mcmap`.

## Troubleshooting

### Compilation fails

Check `g++ --version`. Supported versions are at least 8.0.0.
If your version is not up to date, install a more recent one using your package manager.
You will have access to the new version using `g++-Y` with Y being the version number.
Compile using `CXX=g++-Y make`.

### Compilation fails complaining about OMP something

Try compiling with `OPENMP=NOTHXM8 make`.
This disables the underlying threading code, so performance may drop.

### Output has lines in the ocean

This is due to the merging algorithm.
Try increasing the split size with the `-fragment` option, or change the color of the water block to use the `Full` block type to make it less noticeable.
