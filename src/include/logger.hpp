#ifndef LOGGER_H_
#define LOGGER_H_

#include <spdlog/spdlog.h>

namespace logger {

// Disabled for now
template <typename... Args> void deep_debug(const char *, const Args &...) {}

using spdlog::set_level;

using spdlog::debug;
using spdlog::error;
using spdlog::info;
using spdlog::warn;

} // namespace logger

#endif
