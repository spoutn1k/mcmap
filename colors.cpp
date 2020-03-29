#include "colors.h"
#include "extractcolors.h"
#include "pngreader.h"
#include "globals.h"
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// See header for description
map<string, list<int>> colors;

void loadColors() {
    FILE *f = fopen("colors.json", "r");
	json data = json::parse(f);
	colors = data.get<map<string, list<int>>>();
}
