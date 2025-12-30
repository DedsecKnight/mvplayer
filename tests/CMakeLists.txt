find_package(GTest REQUIRED)

add_executable(test)

target_sources(test PRIVATE multithreaded/utils/queue_test.cpp)
target_link_libraries(test PRIVATE gtest::gtest multithreaded)
