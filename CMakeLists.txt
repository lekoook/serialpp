cmake_minimum_required(VERSION 3.10.0)
project(serialpp
    VERSION 0.1.0
    DESCRIPTION "C++ library for serial ports"
    LANGUAGES C CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_TESTS "Set ON to enable tests." OFF)

# Source files
set(SRCS
    src/serialpp.cpp
)
if(UNIX)
    list(APPEND SRCS src/impl/unix.cpp)
endif()

# Library
add_library(serialpp ${SRCS})
target_include_directories(serialpp PRIVATE src)
target_compile_features(serialpp PUBLIC cxx_std_17)

# Examples
add_executable(list_ports_eg examples/list_ports/list_ports.cpp)
target_include_directories(list_ports_eg PRIVATE src)
target_compile_features(serialpp PUBLIC cxx_std_17)
target_link_libraries(list_ports_eg serialpp)

add_executable(read_write_eg examples/read_write/read_write.cpp)
target_include_directories(read_write_eg PRIVATE src)
target_compile_features(read_write_eg PUBLIC cxx_std_17)
target_link_libraries(read_write_eg serialpp)

add_executable(async_read_write_eg examples/async_read_write/async_read_write.cpp)
target_include_directories(async_read_write_eg PRIVATE src)
target_compile_features(async_read_write_eg PUBLIC cxx_std_17)
target_link_libraries(async_read_write_eg serialpp)

add_executable(subscription_eg examples/subscription/subscription.cpp)
target_include_directories(subscription_eg PRIVATE src)
target_compile_features(subscription_eg PUBLIC cxx_std_17)
target_link_libraries(subscription_eg serialpp)

# Build doxygen docs
add_subdirectory(doc)

# Testing
if (ENABLE_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    enable_testing()
    add_executable(
        tests
        tests/task_queue_test.cpp
    )
    target_link_libraries(
        tests
        GTest::gtest_main
    )
    target_include_directories(tests PRIVATE src)
    include(GoogleTest)
    gtest_discover_tests(tests)
endif()
