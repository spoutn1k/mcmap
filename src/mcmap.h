#ifndef MCMAP_H_
#define MCMAP_H_

#include "./VERSION"
#include "./canvas.h"
#include "./settings.h"
#include <omp.h>
#include <progress.hpp>

namespace mcmap {

int render(const Settings::WorldOptions &, const Colors::Palette &,
           Progress::Callback = Progress::Status::quiet);

std::string version();

std::string compilation_options();

} // namespace mcmap

#endif
