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

  auto operator==(Message const&) const -> bool = default;
};
static_assert(sizeof(Message) == 48);

inline constexpr auto kCacheLine = 64uz;

}  // namespace spsc
