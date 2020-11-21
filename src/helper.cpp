#include "helper.h"
#include <vector>

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

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
  const size_t overhead =
      60 * size_t(1024 * 1024) + std::min(threads, elements) * element_size;
  const size_t required = element_size * elements;
  size_t capacity = std::numeric_limits<size_t>::max();

  if (limit < overhead) {
    logger::error(
        "At least {:.2f}MB are required to render with those parameters\n",
        float(overhead) / float(1024 * 1024));
    return 0;
  }

  if (required > limit) {
    capacity = elements - (required - limit + overhead) / element_size;
  }

  return capacity;
}
