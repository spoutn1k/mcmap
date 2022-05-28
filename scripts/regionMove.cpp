#include "../src/region.h"
#include <logger.hpp>
#include <nbt/parser.hpp>
#include <nbt/to_json.hpp>
#include <translator.hpp>
#include <unordered_set>
#include <zlib.h>

std::unordered_set<nbt::NBT::tag_string_t> natural = {
#include "natural.list"
};

std::unordered_set<nbt::NBT::tag_string_t> prismarine = {
    "minecraft:dark_prismarine",        "minecraft:dark_prismarine_slab",
    "minecraft:dark_prismarine_stairs", "minecraft:prismarine",
    "minecraft:prismarine_brick_slab",  "minecraft:prismarine_brick_stairs",
    "minecraft:prismarine_bricks",      "minecraft:prismarine_slab",
    "minecraft:prismarine_stairs",      "minecraft:prismarine_wall",
};

std::unordered_set<nbt::NBT::tag_string_t> stone = {
    "minecraft:smooth_stone",
    "minecraft:smooth_stone_slab",
    "minecraft:stone",
    "minecraft:stone_brick_slab",
    "minecraft:stone_brick_stairs",
    "minecraft:stone_brick_wall",
    "minecraft:stone_bricks",
    "minecraft:chiseled_stone_bricks",
    "minecraft:polished_andesite",
    "minecraft:polished_andesite_stairs",
    "minecraft:polished_andesite_slab",
};

void strip_naturals(Section &s) {
  if (!s.palette.size()) {
    return;
  }

  for (auto &block : s.blocks) {
    nbt::NBT::tag_string_t type =
        s.palette.at(block)["Name"].get<nbt::NBT::tag_string_t>();

    if (natural.find(type) != natural.end()) {
      block = 0;
    }
  }
}

void dry(Section &s) {
  if (!s.palette.size()) {
    return;
  }

  for (auto &type : s.palette) {
    if (type.contains("Properties") &&
        type["Properties"].contains("waterlogged")) {

      type["Properties"].erase("waterlogged");
    }
  }
}

void bruise(Section &s) {
  if (!s.palette.size())
    return;

  s.palette.emplace(
      s.palette.end(),
      nbt::NBT(nbt::NBT::tag_compound_t(
          {{"", nbt::NBT("minecraft:cracked_stone_bricks", "Name")}})));
  std::vector<nbt::NBT>::size_type cracked_bricks = s.palette.size() - 1;

  s.palette.emplace(
      s.palette.end(),
      nbt::NBT(nbt::NBT::tag_compound_t(
          {{"", nbt::NBT("minecraft:cracked_nether_bricks", "Name")}})));
  std::vector<nbt::NBT>::size_type cracked_nether_bricks = s.palette.size() - 1;

  s.palette.emplace(s.palette.end(),
                    nbt::NBT(nbt::NBT::tag_compound_t(
                        {{"", nbt::NBT("minecraft:cobblestone", "Name")}})));
  std::vector<nbt::NBT>::size_type cobblestone = s.palette.size() - 1;

  s.palette.emplace(s.palette.end(),
                    nbt::NBT(nbt::NBT::tag_compound_t(
                        {{"", nbt::NBT("minecraft:andesite", "Name")}})));
  std::vector<nbt::NBT>::size_type andesite = s.palette.size() - 1;

  s.palette.emplace(s.palette.end(),
                    nbt::NBT(nbt::NBT::tag_compound_t(
                        {{"", nbt::NBT("minecraft:gravel", "Name")}})));
  std::vector<nbt::NBT>::size_type gravel = s.palette.size() - 1;

  for (auto &block : s.blocks) {
    nbt::NBT::tag_string_t type =
        s.palette.at(block)["Name"].get<nbt::NBT::tag_string_t>();

    if (type == "minecraft:stone_bricks" && !(rand() % 3)) {
      block = cracked_bricks;
    }

    if (type == "minecraft:stone_bricks_stairs" && !(rand() % 3)) {
      block = 0;
    }

    if (type == "minecraft:nether_bricks" && !(rand() % 3)) {
      block = cracked_nether_bricks;
    }

    if (prismarine.find(type) != prismarine.end() && !(rand() % 16)) {
      block = 0;
    }

    if (stone.find(type) != stone.end()) {
      int r = rand() % 48;
      switch (r) {
      case 0:
        block = cobblestone;
        break;
      case 1:
        block = andesite;
        break;
      case 2:
        block = gravel;
        break;
      }
    }
  }
}

void process(nbt::NBT &chunk) {
  for (auto &section :
       *chunk["Level"]["Sections"].get<nbt::NBT::tag_list_t *>()) {

    Section sec = Section(section, 2534);

    strip_naturals(sec);
    dry(sec);
    bruise(sec);

    sec.to_nbt(section);
  }
}

int main(int argc, char **argv) {
  logger::set_level(spdlog::level::debug);
  fs::path region_file, target = "r.0.0.mca";

  Region origin, destination("target");
  destination.file = target;

  if (argc < 2 || !fs::exists((region_file = fs::path(argv[1])))) {
    logger::error("Usage: {} <Region file>", argv[0]);
    return 1;
  }

  origin = Region(region_file);

  nbt::NBT chunk;

  for (uint16_t i = 0; i < 32; i++) {
    for (uint16_t j = 0; j < 32; j++) {
      origin.get_chunk({i, j}, chunk);

      process(chunk);

      destination.add_chunk({i, j}, chunk);
      chunk = nbt::NBT();
    }
  }
}
