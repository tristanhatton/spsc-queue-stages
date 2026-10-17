#include "platform.hpp"

#include <chrono>
#include <fstream>
#include <string>

namespace bench {

auto tsc_ticks_per_ns() -> double {
  using Clock = std::chrono::steady_clock;
  auto const wall_start = Clock::now();
  auto const tsc_start = tsc_now();
  while (Clock::now() - wall_start < std::chrono::milliseconds{200}) {
  }
  auto const tsc_end = tsc_now();
  auto const elapsed =
      std::chrono::duration<double, std::nano>{Clock::now() - wall_start};
  return static_cast<double>(tsc_end - tsc_start) / elapsed.count();
}

auto cpu_model() -> std::string {
  auto file = std::ifstream{"/proc/cpuinfo"};
  for (auto line = std::string{}; std::getline(file, line);) {
    if (line.starts_with("model name")) {
      return line.substr(line.find(':') + 2);
    }
  }
  return "unknown";
}

}  // namespace bench
