#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <random>
#include <source_location>
#include <stdexcept>
#include <thread>

#include "message.hpp"
#include "ring_buffer.hpp"

namespace {

auto check(bool const condition, char const* const what,
           std::source_location const where = std::source_location::current())
    -> void {
  if (!condition) {
    std::println(stderr, "{}:{}: check failed in {}: {}", where.file_name(),
                 where.line(), where.function_name(), what);
    std::abort();
  }
}

#define CHECK(condition) check((condition), #condition)

auto make_message(std::uint64_t const seq) -> spsc::Message {
  auto words = std::array<std::uint64_t, 6>{};
  auto x = seq * 0x9e3779b97f4a7c15ull + 0x632be59bd9b4e019ull;
  for (auto& word : words) {
    x ^= x >> 31;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 29;
    word = x;
  }
  return std::bit_cast<spsc::Message>(words);
}

auto rejects_invalid_capacity() -> void {
  for (auto const capacity : {0uz, 3uz, 6uz, 1000uz}) {
    auto threw = false;
    try {
      auto const queue = spsc::RingBuffer{capacity};
    } catch (std::invalid_argument const&) {
      threw = true;
    }
    CHECK(threw);
  }
}

auto fills_and_drains(std::size_t const capacity) -> void {
  auto queue = spsc::RingBuffer{capacity};
  auto out = spsc::Message{};
  CHECK(queue.capacity() == capacity);
  CHECK(queue.size() == 0);
  CHECK(!queue.try_pop(out));
  for (auto lap = std::uint64_t{0}; lap < 3; ++lap) {
    for (auto i = 0uz; i < capacity; ++i) {
      CHECK(queue.try_push(make_message(lap * capacity + i)));
      CHECK(queue.size() == i + 1);
    }
    CHECK(!queue.try_push(make_message(0)));
    for (auto i = 0uz; i < capacity; ++i) {
      CHECK(queue.try_pop(out));
      CHECK(out == make_message(lap * capacity + i));
      CHECK(queue.size() == capacity - i - 1);
    }
    CHECK(!queue.try_pop(out));
  }
}

auto maybe_stall(std::mt19937& rng) -> void {
  if ((rng() & 255) == 0) {
    for (auto spin = rng() & 2047; spin != 0; --spin) {
      asm volatile("" ::: "memory");
    }
  }
}

auto transfers_concurrently(std::size_t const capacity,
                            std::uint64_t const count,
                            unsigned const seed) -> void {
  auto queue = spsc::RingBuffer{capacity};
  {
    auto const consumer = std::jthread{[&] {
      auto rng = std::mt19937{seed};
      auto out = spsc::Message{};
      for (auto n = std::uint64_t{0}; n < count;) {
        if (!queue.try_pop(out)) {
          continue;
        }
        CHECK(out == make_message(n));
        ++n;
        maybe_stall(rng);
      }
    }};
    auto rng = std::mt19937{seed + 1};
    for (auto n = std::uint64_t{0}; n < count; ++n) {
      auto const msg = make_message(n);
      while (!queue.try_push(msg)) {
      }
      maybe_stall(rng);
    }
  }
  CHECK(queue.size() == 0);
}

}  // namespace

auto main() -> int {
  rejects_invalid_capacity();
  for (auto const capacity : {1uz, 2uz, 4uz, 8uz, 64uz, 1024uz}) {
    fills_and_drains(capacity);
  }
  for (auto const capacity : {1uz, 2uz, 4uz, 8uz, 64uz, 1024uz}) {
    for (auto trial = 0u; trial < 2; ++trial) {
      transfers_concurrently(capacity, 500'000, 17 + trial);
    }
  }
  std::println("stress test passed");
  return EXIT_SUCCESS;
}
