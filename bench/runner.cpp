#include "runner.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <thread>
#include <utility>
#include <vector>

#include "platform.hpp"

namespace bench {
namespace {

constexpr auto kWarmUpRound = 0;
constexpr auto kNoRound = -1;
constexpr auto kStop = -2;
constexpr auto kStartDelayTicks = std::uint64_t{200'000};

auto latency_ticks(std::uint64_t const now,
                   std::uint64_t const due) -> std::uint32_t {
  auto const latency = now > due ? now - due : 0;
  return static_cast<std::uint32_t>(std::min<std::uint64_t>(
      latency, std::numeric_limits<std::uint32_t>::max()));
}

}  // namespace

Runner::Runner(Options const& options, Schedule const& schedule)
    : options_(options),
      schedule_(schedule),
      latencies_(schedule.size()),
      queue_(options.capacity),
      round_started_(kNoRound),
      round_finished_(kNoRound) {}

auto Runner::run() -> Stats {
  auto const consumer = std::jthread{[this] { consume(); }};

  produce(kWarmUpRound);
  auto rounds = std::vector<Stats>{};
  rounds.reserve(static_cast<std::size_t>(options_.rounds));
  for (auto round = kWarmUpRound + 1; round <= options_.rounds; ++round) {
    produce(round);
    rounds.push_back(summarize(latencies_));
  }

  round_started_.store(kStop, std::memory_order_release);
  return median_of(std::move(rounds));
}

auto Runner::corrupted() const -> std::uint64_t {
  return corrupted_.load(std::memory_order_relaxed);
}

auto Runner::produce(int const round) -> void {
  auto const start = tsc_now() + kStartDelayTicks;
  start_tsc_.store(start, std::memory_order_relaxed);
  round_started_.store(round, std::memory_order_release);

  for (auto i = 0uz; i < schedule_.size(); ++i) {
    auto const due = start + schedule_[i];
    while (tsc_now() < due) {
    }
    auto const msg = make_message(i, due);
    while (!queue_.try_push(msg)) {
      cpu_relax();
    }
  }
  while (round_finished_.load(std::memory_order_acquire) != round) {
  }
}

auto Runner::consume() -> void {
  for (auto round = wait_for_new_round(kNoRound); round != kStop;
       round = wait_for_new_round(round)) {
    auto const start = start_tsc_.load(std::memory_order_relaxed);
    corrupted_.fetch_add(receive_round(start), std::memory_order_relaxed);
    round_finished_.store(round, std::memory_order_release);
  }
}

auto Runner::wait_for_new_round(int const seen) const -> int {
  auto round = round_started_.load(std::memory_order_acquire);
  while (round == seen) {
    round = round_started_.load(std::memory_order_acquire);
  }
  return round;
}

auto Runner::receive_round(std::uint64_t const start) -> std::uint64_t {
  auto const* const schedule = schedule_.data();
  auto* const latencies = latencies_.data();
  auto const count = schedule_.size();
  auto corrupted = std::uint64_t{0};
  auto msg = spsc::Message{};
  for (auto n = 0uz; n < count; ++n) {
    while (!queue_.try_pop(msg)) {
      cpu_relax();
    }
    auto const now = tsc_after_loads();
    auto const due = start + schedule[n];
    latencies[n] = latency_ticks(now, due);
    corrupted += matches(msg, n, due) ? 0 : 1;
  }
  return corrupted;
}

}  // namespace bench
