#pragma once

#include <bit>
#include <cstddef>
#include <stdexcept>

namespace spsc::detail {

inline auto checked_capacity(std::size_t const capacity) -> std::size_t {
  if (!std::has_single_bit(capacity)) {
    throw std::invalid_argument("capacity must be a nonzero power of two");
  }
  return capacity;
}

}  // namespace spsc::detail
