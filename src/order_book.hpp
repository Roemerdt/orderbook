#pragma once

#include <cassert>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

using Price = std::int64_t;
using Quantity = std::uint64_t;
using OrderId = std::string;

enum class Side {
    Buy,
    Sell
};

enum class RejectReason {
    DuplicatedId,
    UnknownId
};

struct Reject {
    OrderId id;
    RejectReason reason;
    bool operator==(const Reject&) const = default;
};

struct Order {
    OrderId id;
    Side side;
    Price price;
    Quantity quantity;
};

struct Trade {
    OrderId agg_id;
    OrderId res_id;
    Price price;
    Quantity quantity;
    bool operator==(const Trade&) const = default;
};

struct Level {
    Price price;
    Quantity quantity;
    bool operator==(const Level&) const = default;
};

// if reject is set, trades should be empty.
// can enforce this later with a variant type
struct AddResult {
    std::vector<Trade> trades;
    std::optional<Reject> reject;
};

struct CancelResult {
    std::optional<Reject> reject;
};

class OrderBook {
public:
    AddResult add(Order order);
    CancelResult cancel(const OrderId& id);
    std::optional<Level> best_bid() const;
    std::optional<Level> best_ask() const;
private:
    // The level queue must be implemented as a list, since we depend on
    // the fact that its iterator is stable for straightforward dequeues
    using LevelQueue = std::list<Order>;

    struct OrderLocation {
        LevelQueue::iterator it;
        // side/price found through the iterator; valid until erase
    };

    std::map<Price, LevelQueue, std::greater<Price>> bids_;
    std::map<Price, LevelQueue> asks_;
    std::unordered_map<OrderId, OrderLocation> index_;

    template<class BookSide>
    void insert_resting(BookSide& side_map, Order order) {
        OrderId id = order.id;
        auto& level = side_map[order.price];
        level.push_back(std::move(order));
        auto it = std::prev(level.end());
        index_.emplace(std::move(id), OrderLocation{it});
    }

    template<class BookSide>
    void erase_resting(BookSide& side_map, Price price, LevelQueue::iterator it) {
        auto level = side_map.find(price);
        assert(level != side_map.end() && "index-book desynced: level missing for order");
        level->second.erase(it);
        if (level->second.empty()) side_map.erase(level);
    }
};
