#include "colors.h"
#include "block.h"

bool Colors::load(const std::filesystem::path& colorFile, Palette* colors) {
	FILE *f = fopen(colorFile.c_str(), "r");

    try {
	    json data = json::parse(f);
	    *colors = data.get<map<string, list<int>>>();
    } catch (const nlohmann::detail::parse_error& err) {
	    fclose(f);
        fprintf(stderr, "Error parsing color file %s\n", colorFile.c_str());
        return false;
    }

	Block::setColors(*colors);
	fclose(f);

	return true;
}

bool Colors::load(const std::filesystem::path& colorFile, _Palette* colors) {
	FILE *f = fopen(colorFile.c_str(), "r");
    json data;

    try {
	    data = json::parse(f);
    } catch (const nlohmann::detail::parse_error& err) {
	    fclose(f);
        fprintf(stderr, "Error parsing color file %s\n", colorFile.c_str());
        return false;
    }

    auto defList = data.get<map<string, json>>();
    for(auto el : defList) {
        colors->insert(std::pair<string, Colors::_Block*>(el.first, new _Block(el.second)));
    }

	fclose(f);
	return true;
}
