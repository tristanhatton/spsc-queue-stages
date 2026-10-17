#include "ring_buffer.hpp"

#include "capacity.hpp"

namespace spsc {

RingBuffer::RingBuffer(std::size_t const capacity)
    : capacity_(detail::checked_capacity(capacity)), buffer_(capacity_) {}

auto RingBuffer::try_push(Message const& msg) -> bool {
  auto const lock = std::lock_guard{mutex_};
  if (count_ == capacity_) {
    return false;
  }
  buffer_[tail_] = msg;
  tail_ = (tail_ + 1) % capacity_;
  ++count_;
  return true;
}

auto RingBuffer::try_pop(Message& out) -> bool {
  auto const lock = std::lock_guard{mutex_};
  if (count_ == 0) {
    return false;
  }
  out = buffer_[head_];
  head_ = (head_ + 1) % capacity_;
  --count_;
  return true;
}

auto RingBuffer::size() const -> std::size_t {
  auto const lock = std::lock_guard{mutex_};
  return count_;
}

}  // namespace spsc
