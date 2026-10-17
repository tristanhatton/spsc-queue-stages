#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <print>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "message.hpp"
#include "platform.hpp"
#include "ring_buffer.hpp"
#include "workload.hpp"

namespace {

struct Options {
  int rounds = 7;
  std::size_t messages = 400'000;
  std::size_t capacity = 1024;
  std::uint64_t seed = 20261104;
};

struct Stats {
  double mean = 0.0;
  std::uint64_t p50 = 0;
  std::uint64_t p99 = 0;
};

struct Workload {
  char const* name;
  bench::Schedule schedule;
};

auto summarize(std::vector<std::uint32_t> latencies) -> Stats {
  auto sum = 0.0;
  for (auto const latency : latencies) {
    sum += latency;
  }
  auto const percentile = [&](double const q) {
    auto const k = static_cast<std::ptrdiff_t>(
        q * static_cast<double>(latencies.size() - 1));
    std::ranges::nth_element(latencies, latencies.begin() + k);
    return std::uint64_t{latencies[static_cast<std::size_t>(k)]};
  };
  return {sum / static_cast<double>(latencies.size()), percentile(0.50),
          percentile(0.99)};
}

auto median_of(std::vector<Stats> rounds) -> Stats {
  auto const median = [&](auto const member) {
    std::ranges::sort(rounds, {}, member);
    return rounds[rounds.size() / 2].*member;
  };
  return {median(&Stats::mean), median(&Stats::p50), median(&Stats::p99)};
}

class Runner {
 public:
  Runner(Options const& options, bench::Schedule const& schedule)
      : options_(options),
        schedule_(schedule),
        latencies_(schedule.size()),
        queue_(options.capacity) {}

  auto run() -> Stats {
    auto const consumer = std::jthread{[this] { consume(); }};
    auto rounds = std::vector<Stats>{};
    for (auto round = kWarmUpRound; round <= options_.rounds; ++round) {
      produce(round);
      if (round != kWarmUpRound) {
        rounds.push_back(summarize(latencies_));
      }
    }
    round_started_.store(kStop, std::memory_order_release);
    return median_of(std::move(rounds));
  }

  [[nodiscard]] auto corrupted() const -> std::uint64_t {
    return corrupted_.load(std::memory_order_relaxed);
  }

 private:
  static constexpr auto kWarmUpRound = 0;
  static constexpr auto kNoRound = -1;
  static constexpr auto kStop = -2;
  static constexpr auto kStartDelayTicks = std::uint64_t{200'000};

  auto produce(int const round) -> void {
    auto const start = bench::tsc_now() + kStartDelayTicks;
    start_tsc_.store(start, std::memory_order_relaxed);
    round_started_.store(round, std::memory_order_release);

    for (auto i = 0uz; i < schedule_.size(); ++i) {
      auto const due = start + schedule_[i];
      while (bench::tsc_now() < due) {
      }
      auto const msg = bench::make_message(i, due);
      while (!queue_.try_push(msg)) {
        bench::cpu_relax();
      }
    }
    while (round_finished_.load(std::memory_order_acquire) != round) {
    }
  }

  auto consume() -> void {
    auto seen = kNoRound;
    while (true) {
      auto round = round_started_.load(std::memory_order_acquire);
      while (round == seen) {
        round = round_started_.load(std::memory_order_acquire);
      }
      if (round == kStop) {
        return;
      }
      seen = round;

      auto const start = start_tsc_.load(std::memory_order_relaxed);
      auto const* const schedule = schedule_.data();
      auto* const latencies = latencies_.data();
      auto corrupted = std::uint64_t{0};
      auto msg = spsc::Message{};
      for (auto n = 0uz; n < schedule_.size();) {
        if (!queue_.try_pop(msg)) {
          bench::cpu_relax();
          continue;
        }
        auto const now = bench::tsc_after_loads();
        auto const due = start + schedule[n];
        auto const latency = now > due ? now - due : 0;
        latencies[n] = static_cast<std::uint32_t>(std::min<std::uint64_t>(
            latency, std::numeric_limits<std::uint32_t>::max()));
        corrupted += bench::matches(msg, n, due) ? 0 : 1;
        ++n;
      }
      corrupted_.fetch_add(corrupted, std::memory_order_relaxed);
      round_finished_.store(round, std::memory_order_release);
    }
  }

  Options const& options_;
  bench::Schedule const& schedule_;
  std::vector<std::uint32_t> latencies_;

  alignas(spsc::kCacheLine) spsc::RingBuffer queue_;
  alignas(spsc::kCacheLine) std::atomic<int> round_started_ = kNoRound;
  alignas(spsc::kCacheLine) std::atomic<int> round_finished_ = kNoRound;
  alignas(spsc::kCacheLine) std::atomic<std::uint64_t> start_tsc_ = 0;
  alignas(spsc::kCacheLine) std::atomic<std::uint64_t> corrupted_ = 0;
};

auto parse_options(int const argc, char** const argv) -> Options {
  auto options = Options{};
  if (argc % 2 == 0) {
    std::println(stderr, "options take one value each");
    std::exit(EXIT_FAILURE);
  }
  for (auto i = 1; i + 1 < argc; i += 2) {
    auto const key = std::string_view{argv[i]};
    auto const* const value = argv[i + 1];
    if (key == "--rounds") {
      options.rounds = std::max(1, std::atoi(value));
    } else if (key == "--messages") {
      options.messages =
          std::max<std::size_t>(1, std::strtoull(value, nullptr, 10));
    } else if (key == "--capacity") {
      options.capacity = std::strtoull(value, nullptr, 10);
    } else if (key == "--seed") {
      options.seed = std::strtoull(value, nullptr, 10);
    } else {
      std::println(stderr, "unknown option {}", key);
      std::exit(EXIT_FAILURE);
    }
  }
  return options;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  auto const options = parse_options(argc, argv);
  auto const ticks_per_ns = bench::tsc_ticks_per_ns();

  std::println("stage     {}", SPSC_STAGE);
  std::println("cpu       {}", bench::cpu_model());
  std::println("compiler  {}", __VERSION__);
  std::println("tsc       {:.3f} GHz", ticks_per_ns);
  std::println("run       {} messages x {} rounds, capacity {}, seed {}\n",
               options.messages, options.rounds, options.capacity,
               options.seed);
  std::println("{:<8} {:>10} {:>10} {:>10}  (TSC ticks, median of rounds)",
               "workload", "mean", "p50", "p99");

  auto const workloads = std::vector<Workload>{
      {"bursty",
       bench::bursty_schedule(options.messages, ticks_per_ns, options.seed)},
      {"steady", bench::steady_schedule(options.messages, ticks_per_ns)},
  };

  auto passed = true;
  for (auto const& workload : workloads) {
    auto runner = Runner{options, workload.schedule};
    auto const stats = runner.run();
    std::println("{:<8} {:>10.0f} {:>10} {:>10}", workload.name, stats.mean,
                 stats.p50, stats.p99);
    if (runner.corrupted() != 0) {
      std::println(stderr, "FAIL: {}: {} corrupted or reordered messages",
                   workload.name, runner.corrupted());
      passed = false;
    }
  }
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
