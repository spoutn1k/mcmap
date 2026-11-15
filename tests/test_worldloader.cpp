#include "../src/region.h"
#include "../src/worldloader.h"
#include <fstream>
#include <gtest/gtest.h>
#include <translator.hpp>

TEST(TestWorldLoader, TestOversizedChunk) {
  // A chunk claiming more compressed data than the reader's buffer holds, with that much data present, must be refused rather than read
  const fs::path dir = fs::temp_directory_path() / "mcmap_test_worldloader";
  fs::remove_all(dir);
  fs::create_directories(dir);

  const size_t size = COMPRESSED_BUFFER + 50000;
  std::vector<uint8_t> contents(2 * 4096 + 5 + size, 0);

  // Chunk 0 0 starts at sector 2
  translate(contents.data(), uint32_t((2 << 8) | 255));
  translate(contents.data() + 2 * 4096, uint32_t(size + 1));
  contents[2 * 4096 + 4] = 2;

  std::ofstream(dir / "r.0.0.mca", std::ios::binary)
      .write(reinterpret_cast<const char *>(contents.data()), contents.size());

  Colors::Palette palette;
  Terrain::Data world(dir, palette);
  nbt::NBT data;

  ASSERT_FALSE(world.readChunk({0, 0}, data));

  fs::remove_all(dir);
}

TEST(TestWorldLoader, TestReadChunk) {
  const fs::path dir = fs::temp_directory_path() / "mcmap_test_worldloader";
  fs::remove_all(dir);
  fs::create_directories(dir);

  // Chunk -3 5 is in region -1 0
  nbt::NBT chunk(nbt::NBT::tag_compound_t(
      {{"DataVersion", nbt::NBT(int32_t(3100), "DataVersion")}}));
  ASSERT_TRUE(Region(dir / "r.-1.0.mca").add_chunk({-3, 5}, chunk));

  Colors::Palette palette;
  Terrain::Data world(dir, palette);
  nbt::NBT data;

  ASSERT_TRUE(world.readChunk({-3, 5}, data));
  ASSERT_EQ(data["DataVersion"].get<int32_t>(), 3100);
  ASSERT_FALSE(world.readChunk({-4, 5}, data));

  // The game can leave empty region files behind, which hold no chunks
  std::ofstream(dir / "r.0.0.mca", std::ios::binary).close();
  ASSERT_FALSE(world.readChunk({0, 0}, data));

  fs::remove_all(dir);
}
