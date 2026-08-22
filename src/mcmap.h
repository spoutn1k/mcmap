#ifndef MCMAP_H_
#define MCMAP_H_

#include "./VERSION"
#include "./canvas.h"
#include "./settings.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <progress.hpp>

namespace mcmap {

int render(const Settings::WorldOptions &, const Colors::Palette &,
           Progress::Callback = Progress::Status::quiet);

std::string version();

std::map<std::string, std::string> compilation_options();

} // namespace mcmap

#endif
