#ifndef _COLORS_
#define _COLORS_

#include "helper.h"
#include "block.h"
#include <cmath>

// Byte order see below. Colors aligned to word boundaries for some speedup
// Brightness is precalculated to speed up calculations later
// Colors are stored twice since BMP and PNG need them in different order
// Noise is supposed to look normal when -noise 10 is given

// First 256 lines correspond to individual block IDs
// ie colors[0] contains color inforamtions for air
//
// Following 256 lines store variant information
// lines colors[256] to color[263] stores stone variants
//

extern uint8_t colors[512][8];

#define PRED 0
#define PGREEN 1
#define PBLUE 2
#define PALPHA 3
#define NOISE 4
#define BRIGHTNESS 5
#define VINDEX 6	    // Use one of the unused fields to store variant info
			    // Here we store the offset from 255, the last ID
			    // ie Dirt variants begin at 263 -> VINDEX = 263 - 255 = 8
			    // This is for the info to fir on a uint8_t

#define BLOCK_TYPE 7	    // The type of block
			    // Influences the way it is drawn

#define GETBRIGHTNESS(c) (uint8_t)sqrt( \
                                        double(PRED[c]) *  double(PRED[c]) * .236 + \
                                        double(PGREEN[c]) *  double(PGREEN[c]) * .601 + \
                                        double(PBLUE[c]) *  double(PBLUE[c]) * .163)

void loadColors();
bool loadColorsFromFile(const char *file);
bool dumpColorsToFile(const char *file);
bool extractColors(const char *file);
bool loadBiomeColors(const char* path);

uint8_t getId(uint16_t b);
uint8_t getVariant(uint16_t b);
uint8_t setVariant(uint16_t b);
uint8_t* getColor(uint16_t b);

#define	AIR	0
#define	STONE	1
#define	GRASS	2
#define	DIRT	3
#define	COBBLESTONE	4
#define	PLANKS	5
#define	SAPLING	6
#define	BEDROCK	7
#define	FLOWING_WATER	8
#define	WATER	9
#define	FLOWING_LAVA	10
#define	LAVA	11
#define	SAND	12
#define	GRAVEL	13
#define	GOLD_ORE	14
#define	IRON_ORE	15
#define	COAL_ORE	16
#define	LOG	17
#define	LEAVES	18
#define	SPONGE	19
#define	GLASS	20
#define	LAPIS_ORE	21
#define	LAPIS_BLOCK	22
#define	DISPENSER	23
#define	SANDSTONE	24
#define	NOTEBLOCK	25
#define	BED	26
#define	GOLDEN_RAIL	27
#define	DETECTOR_RAIL	28
#define	STICKY_PISTON	29
#define	WEB	30
#define	TALLGRASS	31
#define	DEADBUSH	32
#define	PISTON	33
#define	PISTON_HEAD	34
#define	WOOL	35
#define	YELLOW_FLOWER	37
#define	RED_FLOWER	38
#define	BROWN_MUSHROOM	39
#define	RED_MUSHROOM	40
#define	GOLD_BLOCK	41
#define	IRON_BLOCK	42
#define	DOUBLE_STONE_SLAB	43
#define	STONE_SLAB	44
#define	BRICK_BLOCK	45
#define	TNT	46
#define	BOOKSHELF	47
#define	MOSSY_COBBLESTONE	48
#define	OBSIDIAN	49
#define	TORCH	50
#define	FIRE	51
#define	MOB_SPAWNER	52
#define	OAK_STAIRS	53
#define	CHEST	54
#define	REDSTONE_WIRE	55
#define	DIAMOND_ORE	56
#define	DIAMOND_BLOCK	57
#define	CRAFTING_TABLE	58
#define	WHEAT	59
#define	FARMLAND	60
#define	FURNACE	61
#define	LIT_FURNACE	62
#define	STANDING_SIGN	63
#define	WOODEN_DOOR	64
#define	LADDER	65
#define	RAIL	66
#define	STONE_STAIRS	67
#define	WALL_SIGN	68
#define	LEVER	69
#define	STONE_PRESSURE_PLATE	70
#define	IRON_DOOR	71
#define	WOODEN_PRESSURE_PLATE	72
#define	REDSTONE_ORE	73
#define	LIT_REDSTONE_ORE	74
#define	UNLIT_REDSTONE_TORCH	75
#define	REDSTONE_TORCH	76
#define	STONE_BUTTON	77
#define	SNOW_LAYER	78
#define	ICE	79
#define	SNOW	80
#define	CACTUS	81
#define	CLAY	82
#define	REEDS	83
#define	JUKEBOX	84
#define	FENCE	85
#define	PUMPKIN	86
#define	NETHERRACK	87
#define	SOUL_SAND	88
#define	GLOWSTONE	89
#define	PORTAL	90
#define	LIT_PUMPKIN	91
#define	CAKE	92
#define	UNPOWERED_REPEATER	93
#define	POWERED_REPEATER	94
#define	STAINED_GLASS	95
#define	TRAPDOOR	96
#define	MONSTER_EGG	97
#define	STONEBRICK	98
#define	BROWN_MUSHROOM_BLOCK	99
#define	RED_MUSHROOM_BLOCK	100
#define	IRON_BARS	101
#define	GLASS_PANE	102
#define	MELON_BLOCK	103
#define	PUMPKIN_STEM	104
#define	MELON_STEM	105
#define	VINE	106
#define	FENCE_GATE	107
#define	BRICK_STAIRS	108
#define	STONE_BRICK_STAIRS	109
#define	MYCELIUM	110
#define	WATERLILY	111
#define	NETHER_BRICK	112
#define	NETHER_BRICK_FENCE	113
#define	NETHER_BRICK_STAIRS	114
#define	NETHER_WART	115
#define	ENCHANTING_TABLE	116
#define	BREWING_STAND	117
#define	CAULDRON	118
#define	END_PORTAL	119
#define	END_PORTAL_FRAME	120
#define	END_STONE	121
#define	DRAGON_EGG	122
#define	REDSTONE_LAMP	123
#define	LIT_REDSTONE_LAMP	124
#define	DOUBLE_WOODEN_SLAB	125
#define	WOODEN_SLAB	126
#define	COCOA	127
#define	SANDSTONE_STAIRS	128
#define	EMERALD_ORE	129
#define	ENDER_CHEST	130
#define	TRIPWIRE_HOOK	131
#define	TRIPWIRE	132
#define	EMERALD_BLOCK	133
#define	SPRUCE_STAIRS	134
#define	BIRCH_STAIRS	135
#define	JUNGLE_STAIRS	136
#define	COMMAND_BLOCK	137
#define	BEACON	138
#define	COBBLESTONE_WALL	139
#define	FLOWER_POT	140
#define	CARROTS	141
#define	POTATOES	142
#define	WOODEN_BUTTON	143
#define	SKULL	144
#define	ANVIL	145
#define	TRAPPED_CHEST	146
#define	LIGHT_WEIGHTED_PRESSURE_PLATE	147
#define	HEAVY_WEIGHTED_PRESSURE_PLATE	148
#define	UNPOWERED_COMPARATOR	149
#define	POWERED_COMPARATOR	150
#define	DAYLIGHT_DETECTOR	151
#define	REDSTONE_BLOCK	152
#define	QUARTZ_ORE	153
#define	HOPPER	154
#define	QUARTZ_BLOCK	155
#define	QUARTZ_STAIRS	156
#define	ACTIVATOR_RAIL	157
#define	DROPPER	158
#define	STAINED_HARDENED_CLAY	159
#define	STAINED_GLASS_PANE	160
#define	LEAVES2	161
#define	LOG2	162
#define	ACACIA_STAIRS	163
#define	DARK_OAK_STAIRS	164
#define	SLIME	165
#define	BARRIER	166
#define	IRON_TRAPDOOR	167
#define	PRISMARINE	168
#define	SEA_LANTERN	169
#define	HAY_BLOCK	170
#define	CARPET	171
#define	HARDENED_CLAY	172
#define	COAL_BLOCK	173
#define	PACKED_ICE	174
#define	DOUBLE_PLANT	175
#define	STANDING_BANNER	176
#define	WALL_BANNER	177
#define	DAYLIGHT_DETECTOR_INVERTED	178
#define	RED_SANDSTONE	179
#define	RED_SANDSTONE_STAIRS	180
#define	DOUBLE_STONE_SLAB2	181
#define	STONE_SLAB2	182
#define	SPRUCE_FENCE_GATE	183
#define	BIRCH_FENCE_GATE	184
#define	JUNGLE_FENCE_GATE	185
#define	DARK_OAK_FENCE_GATE	186
#define	ACACIA_FENCE_GATE	187
#define	SPRUCE_FENCE	188
#define	BIRCH_FENCE	189
#define	JUNGLE_FENCE	190
#define	DARK_OAK_FENCE	191
#define	ACACIA_FENCE	192
#define	SPRUCE_DOOR	193
#define	BIRCH_DOOR	194
#define	JUNGLE_DOOR	195
#define	ACACIA_DOOR	196
#define	DARK_OAK_DOOR	197
#define	END_ROD	198
#define	CHORUS_PLANT	199
#define	CHORUS_FLOWER	200
#define	PURPUR_BLOCK	201
#define	PURPUR_PILLAR	202
#define	PURPUR_STAIRS	203
#define	PURPUR_DOUBLE_SLAB	204
#define	PURPUR_SLAB	205
#define	END_BRICKS	206
#define	BEETROOTS	207
#define	GRASS_PATH	208
#define	END_GATEWAY	209
#define	REPEATING_COMMAND_BLOCK	210
#define	CHAIN_COMMAND_BLOCK	211
#define	FROSTED_ICE	212
#define	MAGMA	213
#define	NETHER_WART_BLOCK	214
#define	RED_NETHER_BRICK	215
#define	BONE_BLOCK	216
#define	STRUCTURE_VOID	217
#define	OBSERVER	218
#define	WHITE_SHULKER_BOX	219
#define	ORANGE_SHULKER_BOX	220
#define	MAGENTA_SHULKER_BOX	221
#define	LIGHT_BLUE_SHULKER_BOX	222
#define	YELLOW_SHULKER_BOX	223
#define	LIME_SHULKER_BOX	224
#define	PINK_SHULKER_BOX	225
#define	GRAY_SHULKER_BOX	226
#define	SILVER_SHULKER_BOX	227
#define	CYAN_SHULKER_BOX	228
#define	PURPLE_SHULKER_BOX	229
#define	BLUE_SHULKER_BOX	230
#define	BROWN_SHULKER_BOX	231
#define	GREEN_SHULKER_BOX	232
#define	RED_SHULKER_BOX	233
#define	BLACK_SHULKER_BOX	234
#define	WHITE_GLAZED_TERRACOTTA	235
#define	ORANGE_GLAZED_TERRACOTTA	236
#define	MAGENTA_GLAZED_TERRACOTTA	237
#define	LIGHT_BLUE_GLAZED_TERRACOTTA	238
#define	YELLOW_GLAZED_TERRACOTTA	239
#define	LIME_GLAZED_TERRACOTTA	240
#define	PINK_GLAZED_TERRACOTTA	241
#define	GRAY_GLAZED_TERRACOTTA	242
#define	LIGHT_GRAY_GLAZED_TERRACOTTA	243
#define	CYAN_GLAZED_TERRACOTTA	244
#define	PURPLE_GLAZED_TERRACOTTA	245
#define	BLUE_GLAZED_TERRACOTTA	246
#define	BROWN_GLAZED_TERRACOTTA	247
#define	GREEN_GLAZED_TERRACOTTA	248
#define	RED_GLAZED_TERRACOTTA	249
#define	BLACK_GLAZED_TERRACOTTA	250
#define	CONCRETE	251
#define	CONCRETE_POWDER	252
#define	STRUCTURE_BLOCK	255

#endif
