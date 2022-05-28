#ifndef LOGGER_H_
#define LOGGER_H_

#include <spdlog/spdlog.h>

namespace logger {

using spdlog::set_level;

using spdlog::debug;
using spdlog::error;
using spdlog::info;
using spdlog::trace;
using spdlog::warn;

} // namespace logger

#endif
