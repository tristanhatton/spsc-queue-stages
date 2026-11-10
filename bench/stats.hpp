#pragma once

#include <cstdint>
#include <vector>

namespace bench {

struct Stats {
  double mean = 0.0;
  std::uint64_t p50 = 0;
  std::uint64_t p99 = 0;
};

auto summarize(std::vector<std::uint32_t> latencies) -> Stats;
auto median_of(std::vector<Stats> rounds) -> Stats;

}  // namespace bench
