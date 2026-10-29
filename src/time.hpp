#pragma once

#include <chrono>
#include <cstdint>

inline uint64_t elapsed_ms(std::chrono::steady_clock::time_point ref) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::steady_clock::now() - ref)
      .count();
}

inline uint64_t elapsed_ms(std::chrono::system_clock::time_point ref) {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now() - ref)
      .count();
}
