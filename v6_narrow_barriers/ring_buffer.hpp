#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

#include "message.hpp"

namespace spsc {

class RingBuffer {
 public:
  explicit RingBuffer(std::size_t capacity);
  RingBuffer(RingBuffer const&) = delete;
  auto operator=(RingBuffer const&) -> RingBuffer& = delete;

  [[nodiscard]] auto try_push(Message const& msg) noexcept -> bool;
  [[nodiscard]] auto try_pop(Message& out) noexcept -> bool;
  [[nodiscard]] auto size() const noexcept -> std::size_t;
  [[nodiscard]] auto capacity() const noexcept -> std::size_t {
    return capacity_;
  }

 private:
  struct alignas(kCacheLine) Slot {
    Message message{};
    std::atomic<std::size_t> ticket = 0;
  };
  static_assert(sizeof(Slot) == kCacheLine);

  std::size_t const capacity_;
  std::size_t const mask_;
  std::vector<Slot> slots_;
  alignas(kCacheLine) std::atomic<std::size_t> head_ = 0;
  alignas(kCacheLine) std::size_t tail_ = 0;
  std::size_t cached_head_ = 0;
};

}  // namespace spsc
