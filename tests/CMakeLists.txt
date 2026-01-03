find_package(GTest REQUIRED)

add_executable(test)

target_sources(test PRIVATE 
  multithreaded/utils/queue_test.cpp
  multithreaded/multithreaded_tests.cpp
)
target_link_libraries(test PRIVATE gtest::gtest multithreaded)
