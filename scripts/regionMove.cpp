#include "../src/region.h"
#include <logger.hpp>
#include <nbt/parser.hpp>
#include <translator.hpp>
#include <unordered_set>
#include <zlib.h>

SETUP_LOGGER

void filter(nbt::NBT &chunk) {
  for (auto &section :
       *chunk["Level"]["Sections"].get<nbt::NBT::tag_list_t *>()) {

    auto &palette = section["Palette"];
    std::unordered_set<nbt::NBT::tag_string_t> natural = {
#include "natural.list"
    };

    for (nbt::NBT::size_type i = 0; i < palette.size(); i++) {
      auto &block = palette[i];

      if (natural.find(block["Name"].get<nbt::NBT::tag_string_t>()) !=
          natural.end()) {
        block["Name"] = nbt::NBT("minecraft:air");
      }
    }
  }
}

int main(int argc, char **argv) {
  logger::level = logger::levels::DEEP_DEBUG;
  fs::path region_file, target = "r.0.0.mca";

  Region origin, destination("target");
  destination.file = target;

  if (argc < 2 || !fs::exists((region_file = fs::path(argv[1])))) {
    logger::error("Usage: {} <Region file>\n", argv[0]);
    return 1;
  }

  origin = Region(region_file);

  nbt::NBT chunk;

  for (uint16_t i = 0; i < 32; i++) {
    for (uint16_t j = 0; j < 32; j++) {
      origin.get_chunk({i, j}, chunk);

      filter(chunk);

      destination.add_chunk({i, j}, chunk);
      chunk = nbt::NBT();
    }
  }
}
