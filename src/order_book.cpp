#include "order_book.hpp"
#include <cassert>
#include <numeric>
#include <optional>
#include <vector>

AddResult OrderBook::add(Order order) {
    if (index_.contains(order.id)) {
        return AddResult{{}, Reject{order.id, RejectReason::DuplicatedId}};
    }

    std::vector<Trade> trades;

    if (order.side == Side::Buy) {
        trades = match(asks_, order, [&](Price p) { return p <= order.price; });
        if (order.quantity > 0)
            insert_resting(bids_, std::move(order));
    } else {
        trades = match(bids_, order, [&](Price p) { return p >= order.price; });
        if (order.quantity > 0)
            insert_resting(asks_, std::move(order));
    }

    return {trades, std::nullopt};
}

CancelResult OrderBook::cancel(const OrderId& id) {
    auto location = index_.find(id);
    if (location == index_.end()) {
        return {Reject{id, RejectReason::UnknownId}};
    }

    Side side = location->second.it->side;
    Price price = location->second.it->price;

    if (side == Side::Buy) {
        erase_resting(bids_, price, location->second.it);
    } else {
        erase_resting(asks_, price, location->second.it);
    }

    index_.erase(location);

    return {std::nullopt};
}

std::optional<Level> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;

    const auto& [price, queue] = *bids_.begin();
    Quantity total = std::accumulate(queue.begin(), queue.end(), Quantity{0},
        [](Quantity acc, const Order& o) { return acc + o.quantity; });
    return Level{price, total};
}

std::optional<Level> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;

    const auto& [price, queue] = *asks_.begin();
    Quantity total = std::accumulate(queue.begin(), queue.end(), Quantity{0},
        [](Quantity acc, const Order& o) { return acc + o.quantity; });
    return Level{price, total};
}
