#include "ring_buffer.hpp"

#include "capacity.hpp"
#include "ordering.hpp"

namespace spsc {

RingBuffer::RingBuffer(std::size_t const capacity)
    : capacity_(detail::checked_capacity(capacity)),
      mask_(capacity_ - 1),
      slots_(capacity_) {}

auto RingBuffer::try_push(Message const& msg) noexcept -> bool {
  auto const tail = tail_;
  if (tail - cached_head_ == capacity_) {
    cached_head_ = detail::load_now(head_);
    if (tail - cached_head_ == capacity_) {
      return false;
    }
  }
  auto& slot = slots_[tail & mask_];
  slot.message = msg;
  detail::store_after_writes(slot.ticket, tail + 1, slot.message);
  tail_ = tail + 1;
  return true;
}

auto RingBuffer::try_pop(Message& out) noexcept -> bool {
  auto const head = head_.load(std::memory_order_relaxed);
  auto& slot = slots_[head & mask_];
  if (detail::load_before_reads(slot.ticket, slot.message) != head + 1) {
    return false;
  }
  out = slot.message;
  detail::store_after_reads(head_, head + 1, slot.message);
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
