# `mcmap` - Isometric map visualizer

*Original project by Simon Rettberg. All credits to him for the idea and vision.*

`mcmap` is a tool allowing you to create isometric renders of your Minecraft save file.

![sample](assets/sample.png)

This fork is under heavy development, but compatible with newer versions of Minecraft.

## Usage

__Linux / MacOS__
```
./mcmap <options> ~/.minecraft/saves/<your save>
```

### Options

| Name         | Description                              |
|--------------|------------------------------------------|
|`-from X Z`     |sets the coordinates of the block to start rendering at|
|`-to X Z`       |sets the coordinates of the block to end rendering at|
|`-min/max VAL`  |minimum/maximum Y index (height) of blocks to render|
|`-file NAME`    |sets the output filename to 'NAME'; default is `output.png`|
|`-colors NAME`    |sets the color file to 'NAME'; default is `colors.png`|
|`-nw` `-ne` `-se` `-sw` |controls which direction will point to the top corner; North-West is default|
|`-nowater`      |do not render water|
|`-nether`      |render the nether|
|`-end`          |render the end|

*Note: Currently you need both -from and -to to define bounds.*

## Color file format

The accepted format is a `json` file, with a specific structure. The root contains a list of all the defined [block IDs](https://minecraft.gamepedia.com/Java_Edition_data_values#Blocks), with the namespace prefix, such as `namespace:block`.

#### Simple block

To define a color for a simple, regular block, a list must be provided, the fields meaning the following: `"namespace:block": [RED, GREEN, BLUE, ALPHA, NOISE, BRIGHTNESS]`

Examples:
```
{
    "minecraft:dirt":   [134, 96, 67, 255, 22],
    "minecraft:stone":  [128, 128, 128, 255, 16],
    ...
}
```

#### Complex block

Some blocks are better looking when drawn in a specific way. To specify that a block has to be drawn differently, you have to provide a `json` structure with the fields:

```
{
    "type":     <BlockType>,
    "color":    [RED, GREEN, BLUE, ALPHA, NOISE, BRIGHTNESS],
    "accent":   [RED, GREEN, BLUE, ALPHA, NOISE, BRIGHTNESS] (Optional)
}
```

The different available block types are defined in the file `blockTypes.def`.

Examples:

```
{
    "minecraft:dirt":   [134, 96, 67, 255, 22],
    "minecraft:grass_block": {
        "type":     "Grown",
        "color":    [134, 96, 67, 255, 22],
        "accent":   [102, 142, 62, 255, 14]
    },
    "minecraft:snow": {
        "type":     "Thin",
        "color":    [245, 246, 245, 254, 13]
    }
}
```

## Installation

`mcmap` depends on the `PNG` and `Zlib` libraries. In most cases `PNG` also requires `Zlib`, so you only have to install:


#### `ubuntu`
```
apt install libpng-dev
```

#### `archlinux`
```
pacman -S libpng
```
