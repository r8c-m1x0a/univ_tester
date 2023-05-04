#pragma once

#include <cstdint>

inline uint32_t to_count(uint16_t voltage) {
  uint32_t hz = uint32_t(4000) - uint32_t(4000 - 30) * voltage / 500;
  return uint32_t(10000) * 4000 / hz;
}
