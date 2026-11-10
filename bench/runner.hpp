#pragma once

#include <atomic>
#include <cstdint>
#include <vector>

#include "message.hpp"
#include "options.hpp"
#include "ring_buffer.hpp"
#include "stats.hpp"
#include "workload.hpp"

namespace bench {

class Runner {
 public:
  Runner(Options const& options, Schedule const& schedule);

  auto run() -> Stats;
  [[nodiscard]] auto corrupted() const -> std::uint64_t;

 private:
  auto produce(int round) -> void;
  auto consume() -> void;
  auto wait_for_new_round(int seen) const -> int;
  auto receive_round(std::uint64_t start) -> std::uint64_t;

  Options const& options_;
  Schedule const& schedule_;
  std::vector<std::uint32_t> latencies_;

  alignas(spsc::kCacheLine) spsc::RingBuffer queue_;
  alignas(spsc::kCacheLine) std::atomic<int> round_started_;
  alignas(spsc::kCacheLine) std::atomic<int> round_finished_;
  alignas(spsc::kCacheLine) std::atomic<std::uint64_t> start_tsc_ = 0;
  alignas(spsc::kCacheLine) std::atomic<std::uint64_t> corrupted_ = 0;
};

}  // namespace bench
