#include "colors.h"
#include "block.h"

bool loadColors(colorMap& colors) {
	std::filesystem::path colorFile = "./colors.json";

	if (!std::filesystem::exists(colorFile)) {
		fprintf(stderr, "Color file not found !\n");
		return false;
	}

	FILE *f = fopen(colorFile.c_str(), "r");
	json data = json::parse(f);
	fclose(f);
	colors = data.get<map<string, list<int>>>();
	Block::setColors(colors);

	return true;
}
