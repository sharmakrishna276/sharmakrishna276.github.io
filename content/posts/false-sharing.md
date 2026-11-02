---
title: False sharing: two threads, one cache line
date: 2026/11/02
description: Two threads bumping two unrelated counters ran 4× slower than they should. The fix is one alignas.
---

Here's a puzzle. Two threads each increment *their own* counter 100 million
times. They share no data, so they should scale perfectly. They don't.

```cpp
#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>

constexpr long kIters = 100'000'000;

struct Packed {  // a and b share a cache line
    std::atomic<long> a{0};
    std::atomic<long> b{0};
};

struct Padded {  // a and b live on different cache lines
    alignas(128) std::atomic<long> a{0};
    alignas(128) std::atomic<long> b{0};
};

template <class Counters>
double seconds() {
    Counters c;
    auto start = std::chrono::steady_clock::now();
    std::thread t1([&] { for (long i = 0; i < kIters; ++i) c.a.fetch_add(1, std::memory_order_relaxed); });
    std::thread t2([&] { for (long i = 0; i < kIters; ++i) c.b.fetch_add(1, std::memory_order_relaxed); });
    t1.join();
    t2.join();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

int main() {
    std::printf("packed: %.2fs\n", seconds<Packed>());
    std::printf("padded: %.2fs\n", seconds<Padded>());
}
```

On my Apple M3 (`clang++ -O2`, three runs):

```
packed: 0.73s
padded: 0.18s
```

The only difference is layout, and it makes the packed version **4× slower**.

## What's going on

Caches don't track individual bytes; they track *lines*. A line is the unit
that moves between cores. When a core wants to write, the coherence protocol
(MESI and its relatives) first needs that core to hold the line exclusively,
which invalidates every other core's copy.

In `Packed`, `a` and `b` are 8 bytes apart, so they land in the same line.
Each `fetch_add` on one core steals the line from the other core, and the
line bounces back and forth for the whole run. The threads never touch the
same *variable*, but the hardware sees them fighting over the same *line*.
That's false sharing.

`Padded` puts each counter on its own line, so each core keeps its line in
its own cache and never has to give it up.

## How big is a cache line?

Usually 64 bytes on x86-64, but not always:

- Apple M-series chips use **128-byte** lines (`sysctl hw.cachelinesize`
  prints `128`). With `alignas(64)` the two counters would still collide
  here, which is why the example uses 128.
- C++17 added `std::hardware_destructive_interference_size` in `<new>` for
  this purpose. It's a compile-time guess, though. My clang reports `256`
  for AArch64 because it has to cover every ARM core it might run on, not
  just this one.

If you're shipping to a known machine (and in HFT you always are), check the
real line size and pad with that.

## Where this shows up

- **SPSC queues**: the producer writes `tail`, the consumer writes `head`.
  Put them on the same line and every enqueue slows down the dequeuer.
- **Per-thread stats and counters**: an `std::array<Counter, N>` indexed by
  thread id is a false-sharing machine unless each element is padded.
- **Thread-pool work items** and anything else allocated back-to-back and
  written by different threads.

On Linux, `perf c2c` reports exactly which cache lines are contended between
cores and which code is touching them.

> Lesson: memory layout is part of your concurrency design. Two variables
> that are logically independent but physically adjacent are still shared.
