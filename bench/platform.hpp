#pragma once

#include <x86intrin.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace bench {

inline auto tsc_now() noexcept -> std::uint64_t { return __rdtsc(); }

inline auto cpu_relax() noexcept -> void { _mm_pause(); }

inline auto tsc_after_loads() noexcept -> std::uint64_t {
  std::atomic_signal_fence(std::memory_order_seq_cst);
  _mm_lfence();
  auto const tsc = __rdtsc();
  std::atomic_signal_fence(std::memory_order_seq_cst);
  return tsc;
}

auto tsc_ticks_per_ns() -> double;
auto cpu_model() -> std::string;

}  // namespace bench
