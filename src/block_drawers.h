#ifndef BLOCK_DRAWERS_H_
#define BLOCK_DRAWERS_H_

#include "./canvas.h"
#include "./colors.h"
#include "./nbt/nbt.hpp"

// This obscure typedef allows to create a member function pointer array
// (ouch) to render different block types without a switch case
typedef void (*drawer)(IsometricCanvas *, const uint32_t, const uint32_t,
                       const nbt::NBT &, const Colors::Block *);

// The default block type, hardcoded
void drawFull(IsometricCanvas *, const uint32_t, const uint32_t,
              const nbt::NBT &, const Colors::Block *);

// The other block types are loaded at compile-time from the `blocktypes.def`
// file, with some macro manipulation
#define DEFINETYPE(STRING, CALLBACK)                                           \
  void CALLBACK(IsometricCanvas *, const uint32_t, const uint32_t,             \
                const nbt::NBT &, const Colors::Block *);
#include "./blocktypes.def"
#undef DEFINETYPE

#endif
