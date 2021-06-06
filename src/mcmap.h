#ifndef MCMAP_H_
#define MCMAP_H_

#include "./canvas.h"
#include "./settings.h"
#include <omp.h>
#include <progress.hpp>

#define CACHE "cache"

namespace mcmap {

int render(const Settings::WorldOptions &, const Colors::Palette &,
           progressCallback = Status::quiet);

}

#endif
