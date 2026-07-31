# Orderbook

A limit order book with price-time priority matching. Written as C++ practice. The domain is a simplified version of what runs inside exchanges.

Supports limit orders (add, cancel), aggressive matching against the opposite side, partial fills, and top-of-book queries. Trades execute at the resting order's price. Within a price level, oldest order fills first.

## Layout

The engine is a pure library: `add()` and `cancel()` return result structs (trades, rejects), no IO. Everything observable goes through the return values and `best_bid()` / `best_ask()`.

## Build

Needs CMake >= 3.20 and a C++20 compiler. Catch2 is fetched automatically on first configure, so that one takes a minute.

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/tests
```

Formatting is whatever `.clang-format` says.

`compile_commands.json` ends up in `build/`; symlink it to the repo root if
your editor runs clangd.

## Status

- [x] passive book (add, cancel, level cleanup)
- [x] matching loop, price-time priority
- [ ] `main.cpp`: stdin shell that supports (`ADD <id> <side> <price> <qty>`, `CANCEL <id>`, `TOP`)
- [ ] seeded property/stress test
- [ ] sanitizer run
- [ ] modify(), market orders
- [ ] benchmarking and performance improvements

## Notes

Prices are integer ticks, instead of floats. Order ids are caller-supplied strings, so a duplicate id is a reject. Data structures used: two `std::map<Price, std::list<Order>>` with opposite comparators, so `begin()` is always the best level plus an `unordered_map` from id to list iterator for O(1) cancels. The list is used because its iterators are stable, while a deque's are not.
