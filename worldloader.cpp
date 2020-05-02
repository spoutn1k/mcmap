#include "worldloader.h"

using std::string;
namespace fs = std::filesystem;

void _loadChunksFromRegion(std::filesystem::path, int32_t regionX, int32_t regionZ, Terrain::Data& terrain);
bool _loadChunk(uint32_t offset, FILE* regionData, Terrain::Chunk&, uint8_t&);

NBT* loadChunk(const char *savePath, int32_t x, int32_t z) {
	if (savePath == NULL || *savePath == '\0') {
		return NULL;
	}

	size_t maxlen = strlen(savePath) + 40;
	char *path = new char[maxlen];
	snprintf(path, maxlen, "%s/region/r.%d.%d.mca", savePath, int(x / REGIONSIZE), int(z / REGIONSIZE));
	uint8_t description[COMPRESSED_BUFFER], decompressedBuffer[DECOMPRESSED_BUFFER];
	FILE *rp = fopen(path, "rb");

	if (rp == NULL) {
		printf("Error opening region file %s\n", path);
		return NULL;
	}

	if (fread(description, 1, 4, rp) != 4) {
		printf("Header too short in %s\n", path);
		fclose(rp);
		return NULL;
	}

	uint32_t offset = (_ntohl(description) >> 8) * 4096;
	if (0 != fseek(rp, offset, SEEK_SET)) {
		printf("Error seeking to chunk in region file %s\n", path);
	}
	if (1 != fread(description, 5, 1, rp)) {
		printf("Error reading chunk size from region file %s\n", path);
	}
	uint32_t len = _ntohl(description);
	len--;
	if (fread(description, 1, len, rp) != len) {
		printf("Not enough input for chunk in %s\n", path);
	}
	fclose(rp);

	z_stream zlibStream;
	memset(&zlibStream, 0, sizeof(z_stream));
	zlibStream.next_out = (Bytef*)decompressedBuffer;
	zlibStream.avail_out = DECOMPRESSED_BUFFER;
	zlibStream.avail_in = len;
	zlibStream.next_in = (Bytef*)description;
	inflateInit2(&zlibStream, 32 + MAX_WBITS);

	int status = inflate(&zlibStream, Z_FINISH); // decompress in one step
	inflateEnd(&zlibStream);

	if (status != Z_STREAM_END) {
		printf("Error decompressing chunk from %s\n", path);
	}

	len = zlibStream.total_out;

	bool success;
	NBT *chunk = new NBT(decompressedBuffer, len, success);

	delete[] path;
	return chunk;
}

/* From a set of coordinates,
 * Return a Terrain::Data object containing all of the loaded terrain */
void _loadTerrain(Terrain::Data& terrain, fs::path regionDir) {
    // Parse all the necessary region files
	for (int8_t rx = REGION(terrain.map.minX); rx < REGION(terrain.map.maxX) + 1; rx++) {
		for (int8_t rz = REGION(terrain.map.minZ); rz < REGION(terrain.map.maxZ) + 1; rz++) {
			_loadChunksFromRegion(regionDir, rx, rz, terrain);
		}
	}
}

uint32_t chunk_index(int32_t x, int32_t z, const Terrain::Coordinates& map) {
	return (x - map.minX) + (z - map.minZ)*(map.maxX - map.minX + 1);
}

void _loadChunksFromRegion(fs::path regionDir, int32_t regionX, int32_t regionZ, Terrain::Data& terrain) {
	// First, we try and open the corresponding region file
	FILE *regionData;
	uint8_t regionHeader[REGION_HEADER_SIZE];

	fs::path regionFile = regionDir /= "r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".mca";

	if (!fs::exists(regionFile) || !(regionData = fopen(regionFile.c_str(), "rb"))) {
		printf("Error opening region file %s\n", regionFile.c_str());
		return;
	}

	// Then, we read the header (of size 4K) storing the chunks locations

	if (fread(regionHeader, sizeof(uint8_t), REGION_HEADER_SIZE, regionData) != REGION_HEADER_SIZE) {
		printf("Header too short in %s\n", regionFile.c_str());
		fclose(regionData);
		return;
	}

	// For all the chunks in the file
	for (int it = 0; it < REGIONSIZE*REGIONSIZE; it++) {
		// Bound check
		const int chunkX = (regionX << 5) + (it & 0x1f);
		const int chunkZ = (regionZ << 5) + (it >> 5);
		if (chunkX < terrain.map.minX
				|| chunkX > terrain.map.maxX
				|| chunkZ < terrain.map.minZ
				|| chunkZ > terrain.map.maxZ) {
			// Chunk is not in bounds
			continue;
		}

		//printf("Loading chunk %d %d in r.%d.%d.mca\n", it & 0x1f, it >> 5, regionX, regionZ);
		//printf("Offset: %d\n", _ntohl(regionHeader + it*4) >> 8);

		// Get the location of the data from the header
		const uint32_t offset = (_ntohl(regionHeader + it*4) >> 8) * 4096;
		const uint32_t index = chunk_index(chunkX, chunkZ, terrain.map);

		_loadChunk(offset, regionData, terrain.chunks[index], terrain.heightMap[index]);
	}

	fclose(regionData);
}

uint8_t zData[5*4096];
uint8_t chunkBuffer[1000*1024];

bool _loadChunk(uint32_t offset, FILE* regionData, Terrain::Chunk& destination, uint8_t& heightMap) {
	if (!offset) {
		// Chunk does not exist
		//printf("Chunk does not exist !\n");
		return false;
	}

	if (0 != fseek(regionData, offset, SEEK_SET)) {
		//printf("Error seeking to chunk\n");
		return false;
	}

	// Read the 5 bytes that give the size and type of data
	if (5 != fread(zData, sizeof(uint8_t), 5, regionData)) {
		//printf("Error reading chunk size from region file\n");
		return false;
	}

	uint32_t len = _ntohl(zData);
	//len--; // This dates from Zahl's, no idea of its purpose

	if (fread(zData, sizeof(uint8_t), len, regionData) != len) {
		//printf("Not enough input for chunk\n");
		return false;
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
		return false;
	}

	len = zlibStream.total_out;

	bool success;
	NBT tree = NBT(chunkBuffer, len, success);
	if (!success) {
		printf("The info could not be understood as NBT\n");
		return false;
	}

	// Strip the chunk of pointless sections
	try {
		destination = *(*(tree["Level"]))["Sections"];
		list<NBT_Tag*>* sections;
		destination.getList(sections);

		// Some chunks have a -1 section, we'll pop that real quick
		if (!sections->empty() && !sections->front()->contains("Palette")) {
			sections->pop_front();
		}

		// Pop all the empty top sections
		while (!sections->empty() && !sections->back()->contains("Palette")) {
			sections->pop_back();
		}

		heightMap = (sections->size()) << 4;

	} catch (const std::invalid_argument& e) {
		return false;
	}

	return true;
}

Block Terrain::blockAt(const Terrain::Data& terrain, int32_t x, int32_t z, int32_t y) {
	const uint32_t index = chunk_index(CHUNK(x), CHUNK(z), terrain.map);
	const uint8_t sectionY = y >> 4;
	const uint64_t position = (x & 0x0f) + ((z & 0x0f) + (y & 0x0f)*16)*16;
	try {
		NBT_Tag* section = (terrain.chunks[index])[sectionY];
		if (section->contains("Palette"))
			return Block(getBlockId(position, section));
		return Block("minecraft:air");
	} catch (std::exception& e) {
		printf("Got air because: %s (%d.%d.%d)\n", e.what(), x >> 4, z >> 4, y);
		return Block("minecraft:air");
	}
}

uint16_t heightAt(const Terrain::Data& terrain, int32_t x, int32_t z) {
	const uint64_t index = (CHUNK(x) - terrain.map.minX) + (CHUNK(z) - terrain.map.minZ)*(terrain.map.maxX - terrain.map.minX + 1);
	return 16*(terrain.heightMap[index] >> 4);
}
