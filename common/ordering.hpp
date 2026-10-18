#pragma once

#include <atomic>
#include <cstddef>

#if !defined(__x86_64__)
#error "ordering.hpp relies on the x86-64 (TSO) memory model"
#endif

// On x86-64 every plain load is an acquire and every plain store a release, so
// only the compiler needs constraining. Unlike std::atomic, which GCC treats as
// a barrier for all memory, each helper orders against exactly the memory it
// names (Clang treats asm memory operands as a full barrier, so the benefit is
// GCC-specific). These accesses are outside the C++ memory model, so
// ThreadSanitizer cannot check them.

namespace spsc::detail {

template <class Payload>
auto store_after_writes(std::atomic<std::size_t>& dst, std::size_t const value,
                        Payload const& payload) -> void {
  asm volatile("movq %1, %0" : "=m"(dst) : "r"(value), "m"(payload));
}

template <class Payload>
auto store_after_reads(std::atomic<std::size_t>& dst, std::size_t const value,
                       Payload& payload) -> void {
  asm volatile("movq %2, %0" : "=m"(dst), "+m"(payload) : "r"(value));
}

template <class Payload>
auto load_before_reads(std::atomic<std::size_t> const& src,
                       Payload& payload) -> std::size_t {
  auto value = 0uz;
  asm volatile("movq %2, %0" : "=r"(value), "+m"(payload) : "m"(src));
  return value;
}

// No compiler ordering. Only valid where every later store that must stay
// after this load is conditional on its value (x86 never moves a store above
// an earlier load).
inline auto load_now(std::atomic<std::size_t> const& src) -> std::size_t {
  auto value = 0uz;
  asm volatile("movq %1, %0" : "=r"(value) : "m"(src));
  return value;
}

}  // namespace spsc::detail
