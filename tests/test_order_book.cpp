#include "order_book.hpp"
#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("TestAddExecutesMatch") {
    OrderBook book;

    book.add({"b1", Side::Buy, 1000, 2});
    auto result = book.add({"s1", Side::Sell, 1000, 2});

    CHECK_FALSE(result.reject.has_value());
    REQUIRE(result.trades.size() == 1);
    CHECK(result.trades[0] == Trade{"s1", "b1", 1000, 2});
    CHECK_FALSE(book.best_bid().has_value());
    CHECK_FALSE(book.best_ask().has_value());
}

// Test that if I put something up for sale for 10
// and someone comes along willing to pay 20
// its sold for my resting price of 10
TEST_CASE("TestOrderMatchesPricesCorrectly") {
    OrderBook book;

    book.add({"s1", Side::Sell, 1000, 2});
    auto result = book.add({"b1", Side::Buy, 1200, 2});

    CHECK_FALSE(result.reject.has_value());
    REQUIRE(result.trades.size() == 1);
    CHECK(result.trades[0] == Trade{"b1", "s1", 1000, 2});
    CHECK_FALSE(book.best_bid().has_value());
    CHECK_FALSE(book.best_ask().has_value());
}

// Test that if I put something up for sale at 10 and someone else
// does at 12 and then someone comes and buys for 12, both our asks
// get filled and turn into trades
TEST_CASE("TestOrderMatchesAcrossPriceLevels") {
    OrderBook book;

    book.add({"s1", Side::Sell, 1000, 1});
    book.add({"s2", Side::Sell, 1200, 1});
    book.add({"s3", Side::Sell, 1400, 1});
    auto result = book.add({"b1", Side::Buy, 1200, 2});

    CHECK_FALSE(result.reject.has_value());
    REQUIRE(result.trades.size() == 2);
    CHECK(result.trades[0] == Trade{"b1", "s1", 1000, 1});
    CHECK(result.trades[1] == Trade{"b1", "s2", 1200, 1});
    CHECK(book.best_ask() == Level{1400, 1});
}

// Test that if the incoming order quantity is larger that
// what the book can offer, it consumes everything and the remainder is
// placed into the book to rest
TEST_CASE("TestOrderConsumesEntireSide") {
    OrderBook book;

    book.add({"s1", Side::Sell, 1000, 1});
    book.add({"s2", Side::Sell, 1200, 1});
    book.add({"s3", Side::Sell, 1400, 1});
    auto result = book.add({"b1", Side::Buy, 2000, 10});

    CHECK_FALSE(result.reject.has_value());
    REQUIRE(result.trades.size() == 3);
    CHECK(result.trades[0] == Trade{"b1", "s1", 1000, 1});
    CHECK(result.trades[1] == Trade{"b1", "s2", 1200, 1});
    CHECK(result.trades[2] == Trade{"b1", "s3", 1400, 1});
    CHECK_FALSE(book.best_ask().has_value());
    CHECK(book.best_bid() == Level{2000, 7});

    auto cancel_first_result = book.cancel("s1");
    REQUIRE(cancel_first_result.reject.has_value());
    CHECK(cancel_first_result.reject->reason == RejectReason::UnknownId);
    auto cancel_second_result = book.cancel("s2");
    REQUIRE(cancel_second_result.reject.has_value());
    CHECK(cancel_second_result.reject->reason == RejectReason::UnknownId);
    auto cancel_third_result = book.cancel("s3");
    REQUIRE(cancel_third_result.reject.has_value());
    CHECK(cancel_third_result.reject->reason == RejectReason::UnknownId);
    auto cancel_fourth_result = book.cancel("b1");
    CHECK_FALSE(cancel_fourth_result.reject.has_value());
}

// Test that the oldest order in the level queue is filled by an incoming
// match
TEST_CASE("TestExecuteOldestOrderFirst") {
    OrderBook book;

    book.add({"s1", Side::Sell, 1000, 1});
    book.add({"s2", Side::Sell, 1000, 1});
    auto result = book.add({"b1", Side::Buy, 1000, 1});

    CHECK_FALSE(result.reject.has_value());
    REQUIRE(result.trades.size() == 1);
    CHECK(result.trades[0] == Trade{"b1", "s1", 1000, 1});
    CHECK(book.best_ask() == Level{1000, 1});
}

// Test that the remainder rests at its limit and later arrivals
// at that same limit queue behind it
TEST_CASE("TestIncomingOrderRemainderRestsWithTimePriority") {
    OrderBook book;

    book.add({"s1", Side::Sell, 10100, 5});

    auto agg = book.add({"b2", Side::Buy, 10100, 10});
    REQUIRE(agg.trades.size() == 1);
    CHECK(agg.trades[0] == Trade{"b2", "s1", 10100, 5});
    CHECK(book.best_bid() == Level{10100, 5});
    CHECK_FALSE(book.best_ask().has_value());

    book.add({"b3", Side::Buy, 10100, 5});
    CHECK(book.best_bid() == Level{10100, 10});

    auto probe = book.add({"s2", Side::Sell, 10100, 5});

    REQUIRE(probe.trades.size() == 1);
    CHECK(probe.trades[0] == Trade{"s2", "b2", 10100, 5});
    CHECK(book.best_bid() == Level{10100, 5}); // only b3 remains
}

// Test that if a resting order is partially used, the remainder
// keeps it position in the queue
TEST_CASE("TestRestingOrderRemainingQuantityStaysAtRest") {
    OrderBook book;

    book.add({"s1", Side::Sell, 1000, 9});
    book.add({"s2", Side::Sell, 1000, 7});
    CHECK(book.best_ask() == Level{1000, 16});
    auto first_result = book.add({"b1", Side::Buy, 1000, 5});
    CHECK_FALSE(first_result.reject.has_value());
    REQUIRE(first_result.trades.size() == 1);
    CHECK(first_result.trades[0] == Trade{"b1", "s1", 1000, 5});
    CHECK(book.best_ask() == Level{1000, 11});

    auto second_result = book.add({"b2", Side::Buy, 1000, 4});
    CHECK_FALSE(second_result.reject.has_value());
    REQUIRE(second_result.trades.size() == 1);
    CHECK(second_result.trades[0] == Trade{"b2", "s1", 1000, 4});
    CHECK(book.best_ask() == Level{1000, 7});
}
