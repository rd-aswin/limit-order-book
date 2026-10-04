# High-Performance Limit Order Book & Matching Engine (C++20)

[![Language](https://img.shields.io/badge/Language-C%2B%2B20-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/20)
[![Build System](https://img.shields.io/badge/Build-CMake%20%7C%20Ninja-064F8C?logo=cmake)](https://cmake.org/)
[![Compiler](https://img.shields.io/badge/Compiler-GCC%20%28UCRT64%29-green)](#toolchain--build)
[![Status](https://img.shields.io/badge/Status-V1%20Baseline%20Complete%20%7C%20V2%20WIP-orange)](#implementation-status--integrity-ledger)
[![Architecture](https://img.shields.io/badge/Architecture-Zero--Allocation%20Refactor-critical)](#2-evolutionary-architecture-v1-baseline-vs-v2-wip)

A deterministic, single-threaded Limit Order Book (LOB) and continuous double-auction matching engine implemented in modern C++20. 

This project studies low-latency systems engineering from first principles: moving from an algorithmic baseline built on standard C++ containers to a hardware-conscious, zero-allocation architecture optimized for x86-64 cache hierarchies.

---

## 1. Executive Summary & Market Mechanics

The matching engine enforces continuous **Price-Time Priority (FIFO)** execution:

* **Fixed-Point Discrete Pricing:** Prices and quantities are modeled strictly as discrete integer ticks (`int` / `uint64_t`). Floating-point arithmetic (`float`, `double`) is strictly excluded from the matching pipeline to prevent IEEE 754 precision drift and non-deterministic rounding errors.
* **Deterministic Matching Rules:**
  * **Crossing Aggressive Orders:** Incoming orders that cross the spread match immediately against the best resting liquidity on the opposite side of the book at the maker's resting price.
  * **Resting Passive Orders:** Non-crossing orders rest on their respective price ladders (Bids sorted descending, Asks sorted ascending).
  * **FIFO Queue Priority:** Orders resting at identical price ticks execute in strict chronological arrival order.
  * **Order Lifecycle Support:** Full fills, partial fills with remaining quantity retention, order cancellation, and quantity reduction.
* **Execution Paradigm:** Single-threaded execution loop designed for mechanical sympathy on x86 hardware without runtime memory allocations or thread context switching overhead.

---

## 2. Evolutionary Architecture: V1 Baseline vs. V2 (WIP)

This project strictly adheres to the systems engineering discipline: **"Make it work, make it right, make it fast."** Rather than prematurely applying speculative optimizations, we first built and verified an algorithmic reference engine (V1), benchmarked its empirical baseline, identified hardware bottlenecks, and are now actively refactoring the hot path into a zero-allocation architecture (V2).

```
V1 Baseline (STL Containers)
┌────────────────────────────┐     ┌────────────────────────────┐
│ std::unordered_map<id,Ord> │     │ std::map<Price, deque<id>> │
│ (Heap nodes, hash overhead)│     │ (Red-Black tree, O(log N)) │
└────────────────────────────┘     └────────────────────────────┘
               │                                  │
               ▼                                  ▼
V2 Zero-Allocation Refactor (Contiguous Pre-Allocated Memory - In Progress)
┌────────────────────────────┐     ┌────────────────────────────┐
│ std::vector<Order> pool    │     │ std::vector<Level> ladder  │
│ (1M pre-allocated slots)   │     │ (Direct price indexing)    │
└────────────────────────────┘     └────────────────────────────┘
               │                                  │
               └─────────► Intrusive DLL ◄────────┘
                   (prevOrderId / nextOrderId)
```

### Architectural Comparison Matrix

| Subsystem | V1 Baseline (Complete & Verified) | V2 Zero-Allocation Engine (In Active Progress) | Systems Engineering Justification |
| :--- | :--- | :--- | :--- |
| **Order Storage** | `std::unordered_map<int, Order>` | Pre-allocated `orderPool` (1,000,000 slots) | Eliminates hash bucket collisions, dynamic rehashing, and node allocations. Order ID maps directly to an array index in $O(1)$ time. |
| **Price Ladder** | `std::map<int, std::deque<int>>` | Direct-Indexed array (`priceLevelBid/Ask`) | Eliminates Red-Black tree pointer chasing across RAM ($O(\log N)$). Price ticks map directly to pre-allocated contiguous memory offsets. |
| **Time-Priority Queue** | `std::deque<int>` | **Intrusive Doubly-Linked List** | Standard `std::deque` allocates memory chunks on the heap. Intrusive lists embed traversal indices (`nextOrderId`, `prevOrderId`) directly in the `Order` struct. |
| **Order Cancellation** | Linear search via `std::remove` | $O(1)$ intrusive list unlink | Eliminates $O(N)$ deque traversal. Cancelling an order unhooks its neighbors directly by index in ~2 CPU instructions. |
| **Runtime Allocations** | Frequent `malloc`/`free` under trade load | **0 bytes during active trading** | All memory is allocated once at startup. Zero runtime memory allocation jitter. |
| **Measured Latency** | **~489 nanoseconds** (Active Matching) | **< 50 nanoseconds (Target)** | Minimizes L1/L2 cache misses by laying out structures in contiguous memory. |

---

## 3. Implementation Status & Integrity Ledger

To maintain complete engineering honesty and defensibility, the ledger below explicitly documents what is fully implemented and tested versus what is actively being engineered:

| Subsystem / Capability | Architectural Tier | Status | Verification & Test Evidence |
| :--- | :--- | :--- | :--- |
| **Fixed-Point Discrete Ticks** | V1 Core | **Complete & Verified** | Verified via deterministic test cases; zero floating-point math. |
| **Price-Time Priority (FIFO)** | V1 Core | **Complete & Verified** | Verified in automated test harness (`test_case()` passing with assertions). |
| **Full Executions & Partial Fills** | V1 Core | **Complete & Verified** | Tested across single fills and multi-order resting sweeps. |
| **Order Cancellation & Reduction** | V1 Core | **Complete & Verified** | Verified quantity reduction and order removal. |
| **Headless Benchmark Harness** | V1 Core | **Complete & Verified** | 100,000 synthetic order loop isolating raw engine latency down to nanoseconds. |
| **Pre-Allocated Order Pool** | V2 Refactor | **Implemented** | 1,000,000-order contiguous buffer pre-allocated on the heap at startup (`orderPool.resize()`). |
| **Intrusive Queue (`addOrder`)** | V2 Refactor | **Implemented** | Intrusive doubly linked list logic linking orders at each `PriceLevel` without pointers. |
| **Direct Price Array Indexing** | V2 Refactor | **In Active Development** | Replaced `std::map` with bounded vector levels; active debugging of BBO cursor tracking. |
| **Multi-Level Matching Sweeps** | V2 Refactor | **In Active Debugging** | Resolving price level advancement when top-of-book levels are depleted during crossing matches. |
| **Hardware RDTSC Cycle Timing** | V3 Optimization | **Planned Roadmap** | Sub-nanosecond CPU cycle counter instrumentation using x86 `__rdtsc`. |

---

## 4. Empirical Benchmarking & Profiling Discoveries

### Benchmarking Environment
* **Platform:** x86-64 Architecture, Windows 11
* **Compiler & Toolchain:** GCC 16.1.0 (MSYS2 UCRT64), CMake 3.20+, Ninja 1.13+
* **Optimization Flags:** `-std=c++20 -O3 -Wall -Wextra -Wpedantic`
* **Timing Mechanism:** `std::chrono::high_resolution_clock`

### Profiling Discovery: The Cost of Hot-Path I/O
During initial benchmark runs, an unoptimized test run clocked at **~63,000 nanoseconds (~63 µs)** per order. Profiling revealed the primary bottleneck:
```cpp
// ANTI-PATTERN on the critical hot path:
std::cout << "put to rest in order book." << std::endl;
```
* **The Root Cause:** Synchronous console I/O forces the CPU to pause execution, transition across kernel boundaries, lock terminal buffers, and flush characters via OS system calls (`std::endl`).
* **The Fix:** Console I/O was completely excised from the core matching and cancellation path.
* **Empirical Speedup:** Removing terminal I/O dropped order latency from **63,000 ns to 105 ns** for passive queue insertions (~600x improvement), and established a realistic active matching baseline of **~489 ns**.

---

## 5. Verification & Testing

Functional correctness is verified through automated regression assertions in `lob.cpp`:

1. **Passive Resting Depth:** Verifies that non-crossing buy and sell limit orders correctly populate the book.
2. **Partial Execution:** Verifies that incoming orders partially match against resting liquidity, accurately deduct resting volume, and update order quantities.
3. **Dual-Ladder Matching:** Verifies that incoming sell orders cross against top bids and execute at the maker's price.
4. **Order Count Invariants:** Verifies that active order counts increment on insertion and decrement upon full execution or cancellation.

---

## 6. Toolchain & Build Instructions

### Prerequisites
* CMake $\ge$ 3.20
* Ninja build system
* C++20 compliant compiler: GCC (MSYS2 UCRT64) or Clang

### Building the Project

```powershell
# 1. Generate Ninja build files with Release optimizations (-O3)
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2. Compile the executable
ninja -C build

# 3. Run the automated test and benchmark suite
./build/lob_test.exe
```

---

## 7. Key Engineering Insights

1. **Mechanical Sympathy Outweighs Big-O:** An algorithmic $O(1)$ hash map or $O(\log N)$ tree is severely penalized if nodes are scattered across RAM, causing CPU cache misses (~60–80 ns per DRAM access). Contiguous memory layouts maximize L1/L2 hardware prefetcher efficiency.
2. **Zero Allocation in the Hot Path:** Memory allocators (`malloc`/`free`) involve OS heap tracking and locks. Pre-allocating static pools at startup guarantees deterministic execution times during active trading.
3. **Reference Semantics Matter:** Copying order structs (`Order placedOrder = orderPool[id];`) creates stale snapshots. In-place matching and quantity deductions require strict reference semantics (`Order& placedOrder = ...`) or direct index indexing.
4. **Intrusive Lists vs. Generic Containers:** Embedding `prevOrderId` and `nextOrderId` directly inside the domain struct eliminates list wrapper nodes, keeping all order metadata tightly packed in memory.

---

## 8. Current Roadmap

- [x] **Phase 1 (V1 Baseline):** Dual-ladder continuous matching engine with FIFO price-time priority in C++20 (~489 ns baseline).
- [ ] **Phase 2 (V2 Zero-Allocation - Current Focus):** Complete intrusive doubly linked list integration, resolve dynamic `bestBid`/`bestAsk` level advancement, and reach target latency of < 50 ns.
- [ ] **Phase 3 (Hardware Profiling):** Instrument calibrated `__rdtsc` CPU cycle measurements and latency histograms (p50, p99, p99.9).
- [ ] **Phase 4 (Order Types):** Expand matching core to support Immediate-or-Cancel (IOC) and Fill-or-Kill (FOK) order types.
