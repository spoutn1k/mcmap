#include "./canvas.h"
#include "./settings.h"
#include <omp.h>

#define CACHE "cache"

namespace mcmap {

int render(const Settings::WorldOptions &, const Colors::Palette &);

}
