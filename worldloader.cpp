#include "worldloader.h"

using std::string;
namespace fs = std::filesystem;

uint8_t zData[5*4096];
uint8_t chunkBuffer[1000*1024];

void Terrain::Data::load(const fs::path& regionDir) {
    // Parse all the necessary region files
	for (int8_t rx = REGION(map.minX); rx < REGION(map.maxX) + 1; rx++) {
		for (int8_t rz = REGION(map.minZ); rz < REGION(map.maxZ) + 1; rz++) {
            fs::path regionFile = fs::path(regionDir)
                /= "r."
                    + std::to_string(rx)
                    + "."
                    + std::to_string(rz)
                    + ".mca";

            if (!fs::exists(regionFile)) {
                fprintf(stderr, "Region file %s does not exist, skipping ..\n", regionFile.c_str());
                continue;
            }

            loadRegion(regionFile, rx, rz);
        }
	}
}

void Terrain::Data::loadRegion(const fs::path& regionFile, const int regionX, const int regionZ) {
    FILE *regionHandle;
    uint8_t regionHeader[REGION_HEADER_SIZE];

    if (!(regionHandle = fopen(regionFile.c_str(), "rb"))) {
        printf("Error opening region file %s\n", regionFile.c_str());
        return;
    }
    // Then, we read the header (of size 4K) storing the chunks locations

    if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionHandle) != REGION_HEADER_SIZE) {
        printf("Header too short in %s\n", regionFile.c_str());
        fclose(regionHandle);
		return;
	}

	// For all the chunks in the file
	for (int it = 0; it < REGIONSIZE*REGIONSIZE; it++) {
		// Bound check
		const int chunkX = (regionX << 5) + (it & 0x1f);
		const int chunkZ = (regionZ << 5) + (it >> 5);
		if (chunkX < map.minX
				|| chunkX > map.maxX
				|| chunkZ < map.minZ
				|| chunkZ > map.maxZ) {
			// Chunk is not in bounds
			continue;
		}

		// Get the location of the data from the header
		const uint32_t offset = (_ntohl(regionHeader + it*4) >> 8) * 4096;

		loadChunk(offset, regionHandle, chunkX, chunkZ);
	}

	fclose(regionHandle);
}

void Terrain::Data::loadChunk(const uint32_t offset, FILE* regionHandle, const int chunkX, const int chunkZ) {
	if (!offset) {
		// Chunk does not exist
		//printf("Chunk does not exist !\n");
		return;
	}

	if (0 != fseek(regionHandle, offset, SEEK_SET)) {
		//printf("Error seeking to chunk\n");
		return;
	}

	// Read the 5 bytes that give the size and type of data
	if (5 != fread(zData, sizeof(uint8_t), 5, regionHandle)) {
		//printf("Error reading chunk size from region file\n");
		return;
	}

	uint32_t len = _ntohl(zData);
	//len--; // This dates from Zahl's, no idea of its purpose

	if (fread(zData, sizeof(uint8_t), len, regionHandle) != len) {
		//printf("Not enough input for chunk\n");
		return;
	}

	z_stream zlibStream;
	memset(&zlibStream, 0, sizeof(z_stream));
	zlibStream.next_in = (Bytef*)zData;
	zlibStream.next_out = (Bytef*)chunkBuffer;
	zlibStream.avail_in = len;
	zlibStream.avail_out = 1000*1024;
	inflateInit2(&zlibStream, 32 + MAX_WBITS);

	int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
	inflateEnd(&zlibStream);

	if (status != Z_STREAM_END) {
		printf("Error decompressing chunk: %s\n", zError(status));
		return;
	}

	len = zlibStream.total_out;

	bool success;
	NBT tree = NBT(chunkBuffer, len, success);
	if (!success) {
		printf("The info could not be understood as NBT\n");
		return;
	}

	// Strip the chunk of pointless sections
    size_t chunkPos = chunkIndex(chunkX, chunkZ);
	try {
		chunks[chunkPos] = *(*(tree["Level"]))["Sections"];
		list<NBT_Tag*>* sections;
		chunks[chunkPos].getList(sections);

		// Some chunks have a -1 section, we'll pop that real quick
		if (!sections->empty() && !sections->front()->contains("Palette")) {
			sections->pop_front();
		}

		// Pop all the empty top sections
		while (!sections->empty() && !sections->back()->contains("Palette")) {
			sections->pop_back();
		}

        // Complete the cache, to determine the colors to load
        for (auto it : *sections) {
            if (!it->contains("Palette"))
                continue;

		    list<NBT_Tag*>* blocks;
		    string blockID;
            (*it)["Palette"]->getList(blocks);
            for (auto block : *blocks) {
                (*block)["Name"]->getString(blockID);
                cache.insert(std::pair<std::string, std::list<int>>(blockID, {}));
            }
        }

        uint8_t chunkHeight = sections->size() << 4;
		heightMap[chunkPos] = chunkHeight;

        // If the chunk's height is the highest found, record it
        if (chunkHeight > (heightBounds & 0xf0))
            heightBounds = chunkHeight | (heightBounds & 0x0f);

	} catch (const std::invalid_argument& e) {
        fprintf(stderr, "Err: %s\n", e.what());
		return;
	}
}

size_t Terrain::Data::chunkIndex(int64_t x, int64_t z) const {
	return (x - map.minX) + (z - map.minZ)*(map.maxX - map.minX + 1);
}

Block Terrain::Data::block(const int32_t x, const int32_t z, const int32_t y) const {
	const size_t index = chunkIndex(CHUNK(x), CHUNK(z));
	const uint8_t sectionY = y >> 4;
	const uint64_t position = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f)*16)*16;
	try {
		NBT_Tag* section = (chunks[index])[sectionY];
		if (section->contains("Palette"))
			return Block(getBlockId(position, section));
		return Block("minecraft:air");
	} catch (std::exception& e) {
		printf("Got air because: %s (%d.%d.%d)\n", e.what(), x >> 4, z >> 4, y);
		return Block("minecraft:air");
	}
}

uint8_t Terrain::Data::maxHeight() const {
    return heightBounds & 0xf0;
}

uint8_t Terrain::Data::maxHeight(const int64_t x, const int64_t z) const {
    return heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0xf0;
}

uint8_t Terrain::Data::minHeight() const {
    return (heightBounds & 0x0f) << 4;
}

uint8_t Terrain::Data::minHeight(const int64_t x, const int64_t z) const {
    return (heightMap[chunkIndex(CHUNK(x), CHUNK(z))] & 0x0f) << 4;
}
