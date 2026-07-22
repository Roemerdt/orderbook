#include <catch2/catch_test_macros.hpp>
#include "order_book.hpp"

TEST_CASE("TestNewBookHasNoBidNorAsk") {
    OrderBook book;
    CHECK_FALSE(book.best_bid().has_value());
    CHECK_FALSE(book.best_ask().has_value());
}

TEST_CASE("TestAddEntersOrderBook") {
    OrderBook book;
    auto result = book.add({"b1", Side::Buy, 10000, 30});
    CHECK_FALSE(result.reject.has_value());
    CHECK(result.trades.empty());
    CHECK(book.best_bid() == Level{10000, 30});
}

TEST_CASE("TestAddWithDuplicateOrderIdFails") {
    OrderBook book;
    auto first_result = book.add({"b1", Side::Buy, 1, 1});
    auto duplicate_result = book.add({"b1", Side::Sell, 500, 9});
    CHECK_FALSE(first_result.reject.has_value());
    REQUIRE(duplicate_result.reject.has_value());
    CHECK(duplicate_result.reject->reason == RejectReason::DuplicatedId);
    CHECK(duplicate_result.trades.empty());
    CHECK(book.best_bid() == Level{1, 1});
    CHECK_FALSE(book.best_ask().has_value());
}

TEST_CASE("TestCancelLeavesOrderBook") {
    OrderBook book;
    auto result = book.add({"b1", Side::Buy, 10000, 30});
    CHECK(book.best_bid() == Level{10000, 30});
    book.cancel("b1");
    CHECK_FALSE(book.best_bid().has_value());
}

TEST_CASE("TestCancelOnlyOrderRemovesPriceLevel") {
    OrderBook book;
    book.add({"b1", Side::Buy, 10000, 30});
    book.add({"b2", Side::Buy, 9900, 10});
    book.cancel("b1");
    CHECK(book.best_bid() == Level{9900, 10});
}

TEST_CASE("TestCancelWithUnknownOrderIdFails") {
    OrderBook book;
    auto result = book.cancel("b1");
    REQUIRE(result.reject.has_value());
    CHECK(result.reject->reason == RejectReason::UnknownId);
}
