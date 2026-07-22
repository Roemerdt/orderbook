#include <catch2/catch_test_macros.hpp>
#include "order_book.hpp"

TEST_CASE("TestNewBookHasNoBidNorAsk") {
    OrderBook book;

    CHECK_FALSE(book.best_bid().has_value());
    CHECK_FALSE(book.best_ask().has_value());
}
