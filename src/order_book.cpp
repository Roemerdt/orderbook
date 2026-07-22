#include "order_book.hpp"
#include <optional>

std::optional<Level> OrderBook::best_bid() const {
    return std::nullopt;
}

std::optional<Level> OrderBook::best_ask() const {
    return std::nullopt;
}
