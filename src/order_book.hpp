#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using Price = std::int64_t;
using Quantity = std::uint64_t;
using OrderId = std::string;

enum class Side {
    Buy,
    Sell
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
};

struct Level {
    Price price;
    Quantity quantity;
};

class OrderBook {
public:
    std::vector<Trade> add(Order order);
    bool cancel(const OrderId& id);
    std::optional<Level> best_bid() const;
    std::optional<Level> best_ask() const;
private:

};
