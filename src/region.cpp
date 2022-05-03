#include "./region.h"

uint32_t _ntohi(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

std::pair<int32_t, int32_t> Region::coordinates(const fs::path &_file) {
  int16_t rX, rZ;
  std::string buffer;
  const char delimiter = '.';

  std::stringstream ss(_file.filename().string());
  std::getline(ss, buffer, delimiter); // This removes the 'r.'
  std::getline(ss, buffer, delimiter); // X in r.X.Z.mca

  rX = atoi(buffer.c_str());

  std::getline(ss, buffer, delimiter); // Z in r.X.Z.mca

  rZ = atoi(buffer.c_str());

  return {rX, rZ};
}

Region::Region(const fs::path &_file) : file(_file) {
  locations.fill(Location());

  std::ifstream regionData(file, std::ifstream::binary);

  for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
    char buffer[4];
    regionData.read(buffer, 4);

    locations[chunk].raw_data = _ntohi((uint8_t *)buffer);

    if (!regionData) {
      logger::error("Error reading `{}` header.", _file.string());
      break;
    }
  }

  regionData.close();
}

void Region::write(const fs::path &_file) {
  std::ofstream out(_file.c_str(), std::ofstream::binary);

  for (uint16_t chunk = 0; chunk < REGIONSIZE * REGIONSIZE; chunk++) {
    uint32_t bytes = _ntohi((uint8_t *)&locations[chunk].raw_data);
    out.write((char *)&bytes, 4);
  }

  std::array<char, 4 * REGIONSIZE * REGIONSIZE> timestamps;
  timestamps.fill(0);

  out.write(&timestamps[0], 4 * REGIONSIZE * REGIONSIZE);
}

size_t Region::get_offset(uint8_t max_size) {
  // Get an offset at which the first empty 4K bytes blocks are free
  std::array<Location, REGIONSIZE *REGIONSIZE> sorted = locations;
  std::sort(sorted.begin(), sorted.end(), Location::order);

  size_t block = 2;
  auto it = sorted.begin();

  while (it != sorted.end()) {
    if (it->offset() != block) {
      logger::deep_debug("Empty region of size {} at offset {}",
                         it->offset() - block, block);
      if (it->offset() - block >= max_size)
        break;
    }

    block = it->offset() + it->size();

    it++;
  }

  return block;
}
