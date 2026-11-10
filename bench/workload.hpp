#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "message.hpp"

namespace bench {

using Schedule = std::vector<std::uint64_t>;

auto bursty_schedule(std::size_t count, double ticks_per_ns,
                     std::uint64_t seed) -> Schedule;
auto steady_schedule(std::size_t count, double ticks_per_ns) -> Schedule;

inline auto make_message(std::uint64_t const seq,
                         std::uint64_t const send_tsc) noexcept
    -> spsc::Message {
  return {
      .send_tsc = send_tsc,
      .sequence = seq,
      .price = static_cast<std::int64_t>(seq * 1'000'003 + 17),
      .quantity = static_cast<std::int64_t>(seq % 977 + 1),
      .order_id = seq * 0x9e3779b97f4a7c15ull,
      .instrument = static_cast<std::uint32_t>(seq * 2654435761u),
      .side = static_cast<std::uint16_t>(seq & 1),
      .flags = static_cast<std::uint16_t>(seq >> 5),
  };
}

inline auto matches(spsc::Message const& msg, std::uint64_t const seq,
                    std::uint64_t const send_tsc) noexcept -> bool {
  return msg == make_message(seq, send_tsc);
}

}  // namespace bench
