# Version 1 (V1) Specification: Core Matching Engine & Algorithmic Baseline

## 1. Executive Summary & Engineering Objective

The primary objective of **Version 1 (V1)** is **Functional & Algorithmic Correctness**.

Following the core systems engineering principle:
> *"Make it work, make it right, make it fast."*

Before applying low-level hardware optimizations (such as custom memory arenas, lock-free concurrency, and SIMD intrinsics), the matching engine must have a provably correct, mathematically sound baseline. V1 establishes the fundamental domain models, FIFO price-time priority rules, crossing mechanics, and comprehensive unit tests.

---

## 2. Scope & Architectural Boundaries

| Feature / Dimension | In V1 Scope | Status / Notes |
| :--- | :--- | :--- |
| **Pricing Model** | **Fixed-Point Integers** | Strictly no floating-point (`float`/`double`). Prices stored as ticks or integer cents (e.g., `uint64_t`). |
| **Execution Priority** | **Price-Time Priority (FIFO)** | Best price executes first; orders at identical prices match strictly in arrival order. |
| **Order Types** | **Limit Orders (GTC)** | "Good 'Til Cancel" resting limit orders only. |
| **Asset Scope** | **Single Instrument** | Dedicated to one order book (e.g., `SYMBOL_1`). No multi-asset routing. |
| **Threading Model** | **Single-Threaded** | Purely deterministic, single-threaded execution. No concurrency race conditions. |
| **Interface** | **Headless / C++ API** | Direct C++ method calls. No GUI, no network sockets. |
| **Memory Pools / Arenas** | **Deferred to V2** | Use clean standard containers or basic arrays to focus strictly on matching correctness. |
| **Lock-Free SPSC Queues** | **Deferred to V3** | Multi-threaded publishing and ring buffers are out of scope for V1. |
| **Binary Protocol Parsers**| **Deferred to V4** | Synthetic order injection via test harnesses only; no live NASDAQ ITCH decoding. |

---

## 3. Domain Models & Data Structures

```
                      +-----------------------------------------+
                      |                 ORDER                   |
                      | --------------------------------------- |
                      | order_id : uint64_t                     |
                      | side     : Side (BUY / SELL)            |
                      | price    : uint64_t (Fixed-point ticks) |
                      | quantity : uint32_t (Contracts / shares)|
                      +-----------------------------------------+
                                           |
                                           v
                 +---------------------------------------------------+
                 |                    PRICE LEVEL                    |
                 | ------------------------------------------------- |
                 | price_level : uint64_t                            |
                 | total_volume: uint64_t                            |
                 | order_queue : FIFO Queue of Orders at this price  |
                 +---------------------------------------------------+
                                           |
                 +-------------------------+-------------------------+
                 |                                                   |
                 v                                                   v
+---------------------------------+                 +---------------------------------+
|           BIDS LADDER           |                 |           ASKS LADDER           |
| (Sorted High Price -> Low Price)|                 | (Sorted Low Price -> High Price)|
+---------------------------------+                 +---------------------------------+
```

### 3.1 Type Definitions
* **`OrderId`:** `uint64_t` (Unique 64-bit identifier per order).
* **`Side`:** `enum class Side { BUY, SELL };`
* **`Price`:** `uint64_t` (Fixed-point integer; represents discrete price ticks, e.g., $100.50 with tick size 0.01 = `10050`).
* **`Quantity`:** `uint32_t` (Discrete count of shares or contracts).

### 3.2 Core Entities
1. **`Order`**:
   * Attributes: `order_id`, `side`, `price`, `quantity`.
2. **`PriceLevel`**:
   * Represents a single price point containing:
     * Aggregate volume resting at this price.
     * A FIFO queue holding the individual orders waiting at this price.
3. **`LimitOrderBook`**:
   * Holds the active **Bids Ladder** (sorted descending: highest buyer first).
   * Holds the active **Asks Ladder** (sorted ascending: lowest seller first).
   * Holds an **Order Lookup Index** mapping `order_id` to its location to allow $O(1)$ cancellations.

---

## 4. Core Operations & Matching State Machine

### 4.1 `add_order(Order incoming)`
When an order arrives, execute the following state machine:

```
                  Incoming Order Arrives
                            │
                            ▼
          Is it a BUY order or a SELL order?
             /                           \
       [ BUY Order ]                 [ SELL Order ]
            │                             │
            ▼                             ▼
Does incoming.price >=        Does incoming.price <= 
Best Ask Price?               Best Bid Price?
     /       \                     /       \
   [YES]     [NO]                [YES]     [NO]
    │          │                  │          │
    ▼          ▼                  ▼          ▼
 MATCH       REST               MATCH       REST
 against    on Bids            against     on Asks
 Asks       Ladder             Bids        Ladder
 Queue                         Queue
```

#### Detailed Matching Invariants:
1. **Aggressive vs. Passive:**
   * If incoming order price crosses or equals the opposite book's best price, it is **Aggressive (Taker)**.
   * Any unfilled remainder after matching rests on the book as **Passive (Maker)**.
2. **Execution Price:**
   * Trades execute at the **resting order's price** (the passive price already established in the book), never at the incoming order's price.
3. **Partial Fills:**
   * If `incoming.quantity < resting.quantity`:
     * Execute trade for `incoming.quantity`.
     * Deduct `incoming.quantity` from `resting.quantity`.
     * Incoming order is completely filled and terminates.
   * If `incoming.quantity >= resting.quantity`:
     * Execute trade for `resting.quantity`.
     * Deduct `resting.quantity` from `incoming.quantity`.
     * Remove the resting order from the book.
     * Advance to the next order in the FIFO queue (or next price level) until `incoming.quantity == 0` or prices no longer cross.

### 4.2 `cancel_order(OrderId order_id)`
* Locate the order using the internal lookup table.
* If found:
  * Remove the order from its associated `PriceLevel` FIFO queue.
  * Reduce the `PriceLevel`'s total aggregate volume.
  * If the price level becomes empty, remove the price level from the ladder.
  * Remove the order from the lookup index.
* If not found: return an error/boolean indicating the order does not exist or was already filled.

### 4.3 `get_bbo()`
* Retrieve the **Best Bid** (highest price and total aggregate volume).
* Retrieve the **Best Ask** (lowest price and total aggregate volume).
* Compute the **Bid-Ask Spread**: `spread = best_ask.price - best_bid.price`.

---

## 5. Deterministic Verification & Unit Test Suite

V1 must pass the following automated unit test cases without failure:

1. **Test 1: Basic Resting Order Insertion**
   * Place non-crossing bids and asks. Verify that `get_bbo()` reflects the expected prices and volumes.
2. **Test 2: Exact Full Match**
   * Place a Sell order for 100 @ 50. Place a Buy order for 100 @ 50.
   * Verify: 100 shares trade at 50; both ladders become empty; BBO is empty.
3. **Test 3: Partial Fill (Incoming Smaller than Resting)**
   * Place Sell order: 100 @ 50.
   * Place Buy order: 40 @ 50.
   * Verify: 40 shares trade at 50; resting Sell order has 60 remaining; Ask ladder top volume is 60.
4. **Test 4: Partial Fill (Incoming Larger than Resting / Multi-Level Sweep)**
   * Place Sell orders: 50 @ 101, 50 @ 102.
   * Place Buy order: 80 @ 102.
   * Verify:
     * 50 shares trade @ 101.
     * 30 shares trade @ 102.
     * 20 shares remain resting on the Ask ladder @ 102.
5. **Test 5: Strict FIFO Queue Priority**
   * Place Sell Order 1: 50 @ 100.
   * Place Sell Order 2: 50 @ 100.
   * Place Buy Order: 50 @ 100.
   * Verify: Trade executes against **Order 1 exclusively**; Order 2 remains untouched.
6. **Test 6: Order Cancellation**
   * Place Buy order: 100 @ 99.
   * Cancel Order.
   * Verify: Order is removed; Bid ladder is empty; subsequent cancel for same ID returns false.
7. **Test 7: Zero Price / Zero Quantity Edge Cases**
   * Verify rejection or graceful error handling when invalid inputs are passed.

---

## 6. V1 Acceptance Criteria

A version 1 implementation is considered complete when:
- [ ] Compiles cleanly under modern C++ (C++20 recommended) with `-Wall -Wextra -Werror`.
- [ ] Uses 100% fixed-point integer math for prices and quantities (zero `float` or `double`).
- [ ] All 7 unit test suites pass deterministically.
- [ ] Code is modular and well-structured, ready for V2 memory arena profiling.
