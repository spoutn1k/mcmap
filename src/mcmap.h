#ifndef MCMAP_H_
#define MCMAP_H_

#include "./VERSION"
#include "./canvas.h"
#include "./diagnostics.h"
#include "./settings.h"
#ifdef _OPENMP
#include <omp.h>
#endif
#include <progress.hpp>

namespace mcmap {

// Problems met while loading the world are recorded in the report, if one is given
int render(const Settings::WorldOptions &, const Colors::Palette &,
           Progress::Callback = Progress::Status::quiet,
           Diagnostics::Report * = nullptr);

std::string version();

std::map<std::string, std::string> compilation_options();

} // namespace mcmap

#endif
