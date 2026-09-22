# Engineer Technical Profile & Competency Assessment

**Subject:** Aswin  
**Target Domain:** Quantitative Development & Ultra-Low-Latency Systems (C++20)  
**Project:** `ultra-low-latency-limit-order-book-cpp`  
**Assessment Date:** September 2026  

---

## 1. Executive Summary & Engineering DNA

| Dimension | Assessment |
| :--- | :--- |
| **Prior Background** | **Micro Development / Embedded Systems:** Strong procedural instincts, comfortable with hardware-adjacent state machines, registers, and execution flow. |
| **Current Trajectory** | Transitioning into Modern Systems C++ (C++20), STL container mechanics, advanced data structures, and algorithmic quantitative systems. |
| **Learning Methodology** | **Just-in-Time, High-Agency Practitioner:** Pulls exact functions and algorithms as needed from first principles. Refuses pre-packaged solutions or AI-generated code. |
| **Core Superpower** | **Tenacious Logic Formation & Iterative Problem Solving:** Built a dual-ladder, FIFO-compliant matching engine from scratch with zero boilerplate. |
| **Primary Growth Area** | **C++ Container Semantics & Systems Tooling:** Navigating STL iterator nuances, value-copy vs. live state synchronization, header modularity (`.hpp`), and CMake build systems. |

---

## 2. Technical Knowledge Baseline

### What is Strong & Established
1. **Procedural Logic & State Modeling:** Because of previous micro-development experience, you naturally think in concrete states: *"An order arrives $\to$ does price cross? $\to$ yes: reduce quantities $\to$ no: place in queue"*. You do not get intimidated by multi-branch business logic.
2. **Algorithmic Resilience:** When confronted with logical bugs (confusing price with order ID, infinite loops, incorrect best-price selection), you systematically tackled and resolved each issue across 4 iterative drafts without giving up.
3. **Intellectual Honesty & Strict Standards:** You deliberately enforce a "no yes-man" rule and refuse to let an AI write code for you, ensuring that 100% of the cognitive work and muscle memory remains yours.

### What is New / Currently Being Learned
1. **Advanced STL Containers:** This project marks your **first time** using:
   * `std::map` (Red-Black tree, ordered price ladder, custom comparators).
   * `std::unordered_map` (Hash table, $O(1)$ order lookup index).
   * `std::deque` / `std::queue` (Double-ended queue, FIFO time-priority queueing).
2. **Standard Library Idioms:** Moving beyond competitive programming's single-file logic into idiomatic C++ patterns (e.g., the erase-remove idiom `vec.erase(remove(...), vec.end())`).
3. **Data Science & Backend Systems:** No prior experience with Supabase or web backend layers; currently ramping up on data science toolchains, mathematical aggregations, and data processing libraries.
4. **C++ Build & Project Tooling:** No prior exposure to multi-file compilation (`.hpp` vs. `.cpp`), CMake build systems, or professional test frameworks (Catch2 / Google Test).

---

## 3. Deep Dive: Observed Error Patterns & Cognitive Traps

Analyzing your code iterations reveals four distinct, recurring patterns in how bugs occurred. Understanding these patterns will prevent 80% of future bugs:

```
                  THE 4 RECURRING ERROR PATTERNS
┌───────────────────────────────┐     ┌───────────────────────────────┐
│ 1. Asymmetric Logic Mirroring │     │  2. Value Snapshot vs. Live   │
│   (Fixing Side 1, forgetting  │     │     (Mutating orderBook but   │
│     to mirror in Side 2)      │     │      reading stale local copy)│
└───────────────┬───────────────┘     └───────────────┬───────────────┘
                │                                     │
                ▼                                     ▼
┌───────────────────────────────┐     ┌───────────────────────────────┐
│ 3. Container Side-Effects     │     │ 4. Control Flow Boundary Gaps │
│   (operator[] auto-insertion, │     │   (Nested 'if' without matching│
│    erasing from middle)       │     │    'else' causing loop freeze)│
└───────────────────────────────┘     └───────────────────────────────┘
```

### Pattern 1: Asymmetric Logic Mirroring (The "Dual-Side" Trap)
* **What Happened:** An order book has two mirror sides: BUY (`side == 1`) and SELL (`side == 2`). 
  * In Draft 2: You fixed trade deductions in Side 1, but left Side 2 with one-sided deductions.
  * In Draft 2: You added cancellation cleanup for Bids, but completely omitted Ask cleanup.
  * In Draft 4: You added `placedOrder.quantity -= matchedOrder.quantity;` to Side 2, but forgot to add it to Side 1.
* **Root Cause:** Focusing intently on getting one half of the algorithm working, then treating the opposite side as a secondary copy-paste afterthought.
* **Prescription:** When implementing symmetrical systems (Buy/Sell, Producer/Consumer, Push/Pop), write both sides concurrently or encapsulate the common matching logic into a single generic function that takes opposite sides as parameters.

### Pattern 2: Stale Snapshot vs. Live State (The Value-Copy Trap)
* **What Happened:** At line 50, you wrote:
  ```cpp
  Order placedOrder = orderBook[orderId]; // By-value copy of the order
  while (placedOrder.quantity > 0) { ... }
  ```
  Inside the loop, helper functions like `reduceQuantity(orderId, ...)` updated the live map `orderBook`, but your local variable `placedOrder` was a frozen snapshot that never updated.
* **Root Cause:** Micro-development often works with direct global variables, memory-mapped I/O, or pointers. In C++, assigning an object creates an independent copy by default.
* **Prescription:** Be explicitly conscious of reference semantics (`Order& placedOrder = orderBook[orderId];`) vs. value semantics, or maintain loop counters as explicit local integers.

### Pattern 3: STL Container Side Effects & Idioms
* **What Happened:**
  * Using `it->first` thinking it was the order ID when it was actually the map key (Price).
  * `deque.erase(remove(...))` missing the second iterator argument `, deque.end()`.
  * `orderBook[non_existent_id]` silently auto-inserting `{0, 0, 0}` dummy orders into the map.
* **Root Cause:** Standard library containers have hidden behaviors that are not obvious to beginners coming from low-level procedural code.
* **Prescription:** Treat STL container member functions as having strict contracts. Never use `map[key]` for lookups unless you intend to insert; use `map.find(key) != map.end()` for querying existence.

### Pattern 4: Control Flow Boundary Gaps (Missing `else` Branches)
* **What Happened:** In Side 2, you wrote:
  ```cpp
  if (!priceLevelBid.empty()) {
      if (it->first >= placedOrder.price) { ... }
      // Missing else branch!
  }
  ```
  When prices did not cross, the loop had no path to exit and spun infinitely.
* **Root Cause:** Designing for the "happy path" (when a match happens) while leaving the "unhappy/passive path" (when prices don't cross) implicit rather than explicit.
* **Prescription:** For every condition inside a `while` loop, explicitly trace: *"What happens if this condition is FALSE? How does the loop advance or terminate?"*

---

## 4. Current Milestone Status

- [x] **Milestone 0: Domain Conceptualization** (Understood Price-Time Priority, BBO, and Spread).
- [x] **Milestone 1: V1 Core Algorithmic Baseline** (Functional matching engine completed in `lob.cpp`):
  - [x] Fixed-point integer pricing.
  - [x] `std::map` sorted price ladders (ascending asks, descending bids via `--end()`).
  - [x] FIFO order queues via `std::deque`.
  - [x] Full execution, partial fills, and multi-order sweeping loops.
  - [x] Clean cancellation and empty-level pruning.

---

## 5. Strategic Growth Trajectory (The Next Steps)

```
[ Phase 1: Verification ] ──► [ Phase 2: Architecture ] ──► [ Phase 3: Systems & Speed ]
   Automated assert()            Split into .hpp / .cpp        Replace std::map with
   test harnesses (no cin)       CMake build automation        zero-allocation memory pools
```

1. **Step 1: Automated Testing with `assert()`**
   * Retire `cin` completely.
   * Write 4 self-contained test functions (`test_full_match`, `test_partial_fill`, `test_multi_sweep`, `test_cancel`) to guarantee zero future regressions.
2. **Step 2: C++ Compilation Model & Modularity**
   * Understand the role of Header files (`.hpp` = interface/blueprint) vs. Source files (`.cpp` = implementation).
   * Split `lob.cpp` cleanly into `Types.hpp`, `Order.hpp`, and `LimitOrderBook.cpp`.
3. **Step 3: Modern Build Tooling (CMake)**
   * Learn why CMake exists by using it to automate the multi-file build process.
   * Turn on strict compiler flags (`-Wall -Wextra -Werror`) and sanitizers (`ASan`/`UBSan`).
4. **Step 4: Mechanical Sympathy (V2 Optimization)**
   * Profile memory allocations in `std::map` and `std::deque`.
   * Replace heap-allocated nodes with static contiguous arrays and intrusive doubly-linked queues.
