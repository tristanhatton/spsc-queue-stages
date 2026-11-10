#include "options.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <string_view>

namespace bench {

auto parse_options(int const argc, char** const argv) -> Options {
  auto options = Options{};
  if (argc % 2 == 0) {
    std::println(stderr, "options take one value each");
    std::exit(EXIT_FAILURE);
  }
  for (auto i = 1; i + 1 < argc; i += 2) {
    auto const key = std::string_view{argv[i]};
    auto const* const value = argv[i + 1];
    if (key == "--rounds") {
      options.rounds = std::max(1, std::atoi(value));
    } else if (key == "--messages") {
      options.messages =
          std::max<std::size_t>(1, std::strtoull(value, nullptr, 10));
    } else if (key == "--capacity") {
      options.capacity = std::strtoull(value, nullptr, 10);
    } else if (key == "--seed") {
      options.seed = std::strtoull(value, nullptr, 10);
    } else {
      std::println(stderr, "unknown option {}", key);
      std::exit(EXIT_FAILURE);
    }
  }
  return options;
}

}  // namespace bench
