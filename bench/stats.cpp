#include "stats.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace bench {

auto summarize(std::vector<std::uint32_t> latencies) -> Stats {
  auto sum = 0.0;
  for (auto const latency : latencies) {
    sum += latency;
  }
  auto const percentile = [&](double const q) {
    auto const k = static_cast<std::ptrdiff_t>(
        q * static_cast<double>(latencies.size() - 1));
    std::ranges::nth_element(latencies, latencies.begin() + k);
    return std::uint64_t{latencies[static_cast<std::size_t>(k)]};
  };
  return {sum / static_cast<double>(latencies.size()), percentile(0.50),
          percentile(0.99)};
}

auto median_of(std::vector<Stats> rounds) -> Stats {
  auto const median = [&](auto const member) {
    std::ranges::sort(rounds, {}, member);
    return rounds[rounds.size() / 2].*member;
  };
  return {median(&Stats::mean), median(&Stats::p50), median(&Stats::p99)};
}

}  // namespace bench
