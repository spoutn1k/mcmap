#include "colors.h"
#include "block.h"

bool loadColors(const std::filesystem::path& colorFile, colorMap& colors) {
	FILE *f = fopen(colorFile.c_str(), "r");

    try {
	    json data = json::parse(f);
	    colors = data.get<map<string, list<int>>>();
    } catch (const nlohmann::detail::parse_error& err) {
	    fclose(f);
        fprintf(stderr, "Error parsing color file %s\n", colorFile.c_str());
        return false;
    }

	Block::setColors(colors);
	fclose(f);

	return true;
}
