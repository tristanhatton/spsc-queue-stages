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
  std::size_t const capacity_;
  std::size_t const mask_;
  std::vector<Message> buffer_;
  std::atomic<std::size_t> head_ = 0;
  std::atomic<std::size_t> tail_ = 0;
};

}  // namespace spsc
