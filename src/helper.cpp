#include "helper.h"
#include <logger.hpp>

uint32_t _ntohl(uint8_t *val) {
  return (uint32_t(val[0]) << 24) + (uint32_t(val[1]) << 16) +
         (uint32_t(val[2]) << 8) + (uint32_t(val[3]));
}

uint8_t clamp(int32_t val) {
  if (val < 0) {
    return 0;
  }
  if (val > 255) {
    return 255;
  }
  return (uint8_t)val;
}

bool isNumeric(const char *str) {
  if (str[0] == '-' && str[1] != '\0') {
    ++str;
  }
  while (*str != '\0') {
    if (*str < '0' || *str > '9') {
      return false;
    }
    ++str;
  }
  return true;
}

size_t memory_capacity(size_t limit, size_t element_size, size_t elements,
                       size_t threads) {
  // Reserve 60K for variables and stuff
  const size_t overhead = 60 * size_t(1024 * 1024);
  // Rendering requires at least this amount
  const size_t rendering = std::min(threads, elements) * element_size;

  // Check we have enought memory
  if (limit < overhead + rendering) {
    logger::error(
        "At least {:.2f}MB are required to render with those parameters",
        float(overhead + rendering) / float(1024 * 1024));
    return 0;
  }

  // Return the amount of canvasses that fit in memory - including the ones
  // being rendered
  return (limit - overhead - rendering) / element_size;
}

bool prepare_cache(const std::filesystem::path &cache) {
  // If we can create the directory, no more checks
  if (create_directory(cache))
    return true;

  fs::file_status cache_status = status(cache);
  fs::perms required = fs::perms::owner_all;

  if (cache_status.type() != fs::file_type::directory) {
    logger::error("Cache directory `{}` is not a directory", cache.string());
    return false;
  }

  if ((cache_status.permissions() & required) != required) {
    logger::error("Cache directory `{}` does not have the right permissions",
                  cache.string());
    return false;
  }

  return true;
}

fs::path getHome() {
#ifndef _WINDOWS
  char target[] = "HOME";
  char *query = getenv(target);

  if (!query)
    return "";

  return std::string(query);
#else
  char drive[] = "HOMEDRIVE", dir[] = "HOMEPATH";
  char *query_drive = getenv(drive), *query_path = getenv(dir);

  if (!query_drive || !query_path)
    return "";

  return fs::path(std::string(query_drive)) / fs::path(std::string(query_path));
#endif
}

fs::path getSaveDir() {
  fs::path prefix, suffix = ".minecraft/saves";

#ifndef _WINDOWS
  prefix = getHome();
#else
  char target[] = "APPDATA";

  char *query = getenv(target);

  if (!query)
    return "";

  prefix = std::string(query);
#endif

  if (!fs::exists(prefix))
    return "";

  return prefix / suffix;
}

fs::path getTempDir() {
  std::error_code error;

  fs::path destination = fs::temp_directory_path(error);

  if (destination.empty() || error)
    destination = fs::current_path();

  return destination / mcmap::config::cache_name;
}
