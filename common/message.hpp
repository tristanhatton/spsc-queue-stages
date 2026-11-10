#pragma once

#include <cstddef>
#include <cstdint>

namespace spsc {

struct Message {
  std::uint64_t send_tsc;
  std::uint64_t sequence;
  std::int64_t price;
  std::int64_t quantity;
  std::uint64_t order_id;
  std::uint32_t instrument;
  std::uint16_t side;
  std::uint16_t flags;

  [[nodiscard]] static constexpr auto from_sequence(
      std::uint64_t const seq,
      std::uint64_t const send_tsc) noexcept -> Message {
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

  auto operator==(Message const&) const -> bool = default;
};
static_assert(sizeof(Message) == 48);

inline constexpr auto kCacheLine = 64uz;

}  // namespace spsc
