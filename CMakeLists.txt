cmake_minimum_required(VERSION 3.20)
project(orderbook CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(orderbook_lib src/order_book.cpp)
target_include_directories(orderbook_lib PUBLIC src)

add_executable(orderbook src/main.cpp)
target_link_libraries(orderbook PRIVATE orderbook_lib)

include(FetchContent)
FetchContent_Declare(catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1)
FetchContent_MakeAvailable(catch2)

add_executable(tests tests/test_order_book.cpp)
target_link_libraries(tests PRIVATE orderbook_lib Catch2::Catch2WithMain)

include(CTest)
add_test(NAME unit COMMAND tests)
