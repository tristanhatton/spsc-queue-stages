#include <atomic>
#include <cstddef>

struct Slot {
  long payload[6];
  std::atomic<std::size_t> ticket;
};

struct Queue {
  Slot* slots;
  std::size_t mask;
  std::size_t tail;
};

Queue* g_queue;

__attribute__((noinline)) auto publish_atomic(int const n) -> void {
  for (auto i = 0; i < n; ++i) {
    auto* const q = g_queue;
    auto& slot = q->slots[q->tail & q->mask];
    slot.payload[0] = i;
    slot.ticket.store(++q->tail, std::memory_order_release);
  }
}

__attribute__((noinline)) auto publish_narrow(int const n) -> void {
  for (auto i = 0; i < n; ++i) {
    auto* const q = g_queue;
    auto& slot = q->slots[q->tail & q->mask];
    slot.payload[0] = i;
    auto const next = ++q->tail;
    asm volatile("movq %1, %0"
                 : "=m"(slot.ticket)
                 : "r"(next), "m"(slot.payload));
  }
}
