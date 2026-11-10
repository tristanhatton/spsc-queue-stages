#pragma once

#include <cstddef>
#include <cstdint>

namespace bench {

struct Options {
  int rounds = 7;
  std::size_t messages = 400'000;
  std::size_t capacity = 1024;
  std::uint64_t seed = 20261104;
};

auto parse_options(int argc, char** argv) -> Options;

}  // namespace bench
