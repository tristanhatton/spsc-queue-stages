#include "ring_buffer.hpp"

#include "capacity.hpp"

namespace spsc {

RingBuffer::RingBuffer(std::size_t const capacity)
    : capacity_(detail::checked_capacity(capacity)),
      mask_(capacity_ - 1),
      slots_(capacity_) {}

auto RingBuffer::try_push(Message const& msg) noexcept -> bool {
  auto const tail = tail_;
  if (tail - cached_head_ == capacity_) {
    cached_head_ = head_.load(std::memory_order_acquire);
    if (tail - cached_head_ == capacity_) {
      return false;
    }
  }
  auto& slot = slots_[tail & mask_];
  slot.message = msg;
  slot.ticket.store(tail + 1, std::memory_order_release);
  tail_ = tail + 1;
  return true;
}

auto RingBuffer::try_pop(Message& out) noexcept -> bool {
  auto const head = head_.load(std::memory_order_relaxed);
  auto const& slot = slots_[head & mask_];
  if (slot.ticket.load(std::memory_order_acquire) != head + 1) {
    return false;
  }
  out = slot.message;
  head_.store(head + 1, std::memory_order_release);
  return true;
}

auto RingBuffer::size() const noexcept -> std::size_t {
  auto const head = head_.load(std::memory_order_acquire);
  auto count = 0uz;
  while (count < capacity_ &&
         slots_[(head + count) & mask_].ticket.load(
             std::memory_order_acquire) == head + count + 1) {
    ++count;
  }
  return count;
}

}  // namespace spsc
