#include "./canvas.h"
#include "./settings.h"
#include <omp.h>

#define CACHE "cache"

int render(const Settings::WorldOptions &, const Colors::Palette &);
