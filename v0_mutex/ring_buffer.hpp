#pragma once

#include <cstddef>
#include <mutex>
#include <vector>

#include "message.hpp"

namespace spsc {

class RingBuffer {
 public:
  explicit RingBuffer(std::size_t capacity);
  RingBuffer(RingBuffer const&) = delete;
  auto operator=(RingBuffer const&) -> RingBuffer& = delete;

  [[nodiscard]] auto try_push(Message const& msg) -> bool;
  [[nodiscard]] auto try_pop(Message& out) -> bool;
  [[nodiscard]] auto size() const -> std::size_t;
  [[nodiscard]] auto capacity() const noexcept -> std::size_t {
    return capacity_;
  }

 private:
  std::size_t const capacity_;
  std::vector<Message> buffer_;
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  std::size_t count_ = 0;
  mutable std::mutex mutex_;
};

}  // namespace spsc
