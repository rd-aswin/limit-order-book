# V2 Limit Order Book Specification: Zero-Allocation Architecture

## 1. Objective and Engineering Context
The goal of the **V2 Limit Order Book** is to achieve **sub-100 nanosecond latency** for order insertion and matching. 

In V1, the baseline latency stabilized at **~489 nanoseconds** under active matching load. Profiling revealed that the vast majority of this latency was consumed by:
1. **Dynamic Memory Management:** `std::map` and `std::deque` utilizing `new` and `delete` to allocate and free nodes on the heap during trade matching.
2. **CPU Cache Thrashing (Pointer Chasing):** Traversing `std::map` Red-Black tree pointers scatters memory access across RAM, causing L1/L2 cache misses.

V2 introduces **Mechanical Sympathy**—aligning our data structures with how modern x86 CPU caches and memory prefetchers actually work.

---

## 2. Technical Stack and Environment Constraints
- **Language Standard:** C++20 (Strict).
- **Compilation:** GCC via MSYS2, `-O3` Release mode, `-Wall -Wextra -Wpedantic`.
- **Memory Model:** Zero-allocation hot path. No heap allocations (`new`, `malloc`, `std::make_shared`, `std::map`, `std::unordered_map`, `std::deque`) are permitted during the trading loop. All memory must be allocated up-front during system initialization.
- **Hardware Assumption:** x86-64 architecture, 64-byte L1 Cache lines.

---

## 3. Core Architectural Transitions

To eliminate heap allocations and cache misses, V2 replaces standard library containers with pre-allocated, flat contiguous arrays.

### A. The Order Pool (Replacing `std::unordered_map`)
Instead of a hash map dynamically creating nodes for every new `orderId`, we pre-allocate a massive array of `Order` structs.

**V1:** `std::unordered_map<int, Order> orderBook;`
**V2:** `std::array<Order, MAX_ORDERS> orderPool;`
- `orderId` simply becomes the index of the array: `orderPool[orderId]`.
- **Complexity:** $O(1)$ memory address calculation. No hashing, no collisions, no allocations.

### B. Price Level Direct Indexing (Replacing `std::map`)
Instead of a Red-Black tree that requires $O(\log N)$ traversal, we use an array where the index is the exact price. (Assuming a known, bounded price range, e.g., 0 to 100,000 ticks).

**V1:** `std::map<int, std::deque<int>> priceLevelBid;`
**V2:** `std::array<PriceLevel, MAX_PRICE> bidLevels;`
- To find price 99: `bidLevels[99]`.
- **Complexity:** $O(1)$ direct memory offset. 0 pointer hops.

### C. Intrusive Queues (Replacing `std::deque`)
Standard `std::deque` dynamically allocates memory in chunks. In V2, we implement **Intrusive Linked Lists**. 
Since we already have the `orderPool` array, we can just add `nextOrderId` and `prevOrderId` integers to the `Order` struct itself. The `PriceLevel` only needs to know the `head` and `tail` integer IDs. 
All "pointers" are just integer indices pointing to other slots in the pre-allocated array.

---

## 4. Architectural Challenges to Solve

Instead of standard library containers, V2 requires you to design custom mechanisms:

### Challenge 1: The Zero-Allocation Order Store
- How do we store up to 1,000,000 active orders without calling `new` or using `std::unordered_map`?
- How do we look up an order by its ID in $O(1)$ time without hashing?

### Challenge 2: The Intrusive FIFO Queue
- In V1, each price level had a `std::deque<int>` to maintain time priority (FIFO).
- In V2, we cannot use `std::deque` because it allocates heap memory.
- How can we link orders together in FIFO order using only pre-allocated arrays and integer indices (an intrusive doubly-linked list)?

### Challenge 3: Direct Price Indexing & Best Level Tracking
- In V1, `std::map` kept prices sorted so `begin()` was the best ask and `rbegin()` was the best bid.
- In V2, if we use a direct-indexed array where index = price, how do we know where the current Best Bid and Best Ask are without searching through all 100,000 price levels every time?


---

## 5. Development Roadmap for V2

We will build V2 incrementally, proving each low-latency concept:

1. **Step 1: The Memory Pool** 
   Replace `std::unordered_map` with `std::array<Order, MAX_ORDERS>`. Convert all `orderId` lookups to direct array indexing.
2. **Step 2: Intrusive Queues** 
   Build the manual linked-list logic (`nextOrderId`, `prevOrderId`) to replace `std::deque`. Test FIFO correctness.
3. **Step 3: Direct Price Indexing** 
   Replace `std::map` with `std::array<PriceLevel, MAX_PRICE>`. Implement logic to track `bestBid` and `bestAsk` manually using integer scanning or bitsets.
4. **Step 4: Benchmarking** 
   Run the identical 100,000 alternating order workload and verify the latency drops from ~489 ns to sub-100 ns.
