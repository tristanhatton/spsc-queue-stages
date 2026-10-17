#include "workload.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>

namespace bench {
namespace {

constexpr auto kMeanBurst = 8;
constexpr auto kMaxBurst = 64;
constexpr auto kMinGapNs = 200.0;
constexpr auto kMeanExtraGapNs = 4800.0;
constexpr auto kSteadyIntervalNs = 200.0;

}  // namespace

auto bursty_schedule(std::size_t const count, double const ticks_per_ns,
                     std::uint64_t const seed) -> Schedule {
  auto rng = std::mt19937_64{seed};
  auto extra_in_burst = std::geometric_distribution<int>{1.0 / kMeanBurst};
  auto extra_gap_ns =
      std::exponential_distribution<double>{1.0 / kMeanExtraGapNs};

  auto schedule = Schedule{};
  schedule.reserve(count);
  auto now_ns = 0.0;
  while (schedule.size() < count) {
    auto const burst = 1 + std::min(extra_in_burst(rng), kMaxBurst - 1);
    auto const at = static_cast<std::uint64_t>(now_ns * ticks_per_ns);
    for (auto k = 0; k < burst && schedule.size() < count; ++k) {
      schedule.push_back(at);
    }
    now_ns += kMinGapNs + extra_gap_ns(rng);
  }
  return schedule;
}

auto steady_schedule(std::size_t const count,
                     double const ticks_per_ns) -> Schedule {
  auto schedule = Schedule(count);
  for (auto i = 0uz; i < count; ++i) {
    schedule[i] = static_cast<std::uint64_t>(static_cast<double>(i) *
                                             kSteadyIntervalNs * ticks_per_ns);
  }
  return schedule;
}

}  // namespace bench
