#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace bench {

using Schedule = std::vector<std::uint64_t>;

auto bursty_schedule(std::size_t count, double ticks_per_ns,
                     std::uint64_t seed) -> Schedule;
auto steady_schedule(std::size_t count, double ticks_per_ns) -> Schedule;

}  // namespace bench
