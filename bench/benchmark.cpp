#include <cstdio>
#include <cstdlib>
#include <print>
#include <string_view>
#include <vector>

#include "options.hpp"
#include "platform.hpp"
#include "runner.hpp"
#include "workload.hpp"

namespace {

struct Workload {
  std::string_view name;
  bench::Schedule schedule;
};

auto print_header(bench::Options const& options,
                  double const ticks_per_ns) -> void {
  std::println("stage     {}", SPSC_STAGE);
  std::println("cpu       {}", bench::cpu_model());
  std::println("compiler  {}", __VERSION__);
  std::println("tsc       {:.3f} GHz", ticks_per_ns);
  std::println("run       {} messages x {} rounds, capacity {}, seed {}\n",
               options.messages, options.rounds, options.capacity,
               options.seed);
  std::println("{:<8} {:>10} {:>10} {:>10}  (TSC ticks, median of rounds)",
               "workload", "mean", "p50", "p99");
}

auto make_workloads(bench::Options const& options,
                    double const ticks_per_ns) -> std::vector<Workload> {
  return {
      {"bursty",
       bench::bursty_schedule(options.messages, ticks_per_ns, options.seed)},
      {"steady", bench::steady_schedule(options.messages, ticks_per_ns)},
  };
}

}  // namespace

auto main(int argc, char** argv) -> int {
  auto const options = bench::parse_options(argc, argv);
  auto const ticks_per_ns = bench::tsc_ticks_per_ns();
  print_header(options, ticks_per_ns);

  auto passed = true;
  for (auto const& workload : make_workloads(options, ticks_per_ns)) {
    auto runner = bench::Runner{options, workload.schedule};
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
