#include "ring_buffer.hpp"

#include <algorithm>

#include "capacity.hpp"

namespace spsc {

RingBuffer::RingBuffer(std::size_t const capacity)
    : capacity_(detail::checked_capacity(capacity)),
      mask_(capacity_ - 1),
      buffer_(capacity_) {}

auto RingBuffer::try_push(Message const& msg) noexcept -> bool {
  auto const tail = tail_.load(std::memory_order_relaxed);
  if (tail - head_.load(std::memory_order_acquire) == capacity_) {
    return false;
  }
  buffer_[tail & mask_] = msg;
  tail_.store(tail + 1, std::memory_order_release);
  return true;
}

auto RingBuffer::try_pop(Message& out) noexcept -> bool {
  auto const head = head_.load(std::memory_order_relaxed);
  if (head == tail_.load(std::memory_order_acquire)) {
    return false;
  }
  out = buffer_[head & mask_];
  head_.store(head + 1, std::memory_order_release);
  return true;
}

auto RingBuffer::size() const noexcept -> std::size_t {
  auto const head = head_.load(std::memory_order_acquire);
  auto const tail = tail_.load(std::memory_order_acquire);
  return std::min(tail - head, capacity_);
}

}  // namespace spsc
