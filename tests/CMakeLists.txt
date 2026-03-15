find_package(GTest REQUIRED)

add_executable(test)

target_sources(test PRIVATE
  multithreaded/utils/queue_test.cpp
  multithreaded/multithreaded_tests.cpp
  multithreaded/engine_tests.cpp
  opengl/opengl_tests.cpp
  mvplayer/renderer_tests.cpp
)
target_link_libraries(test PRIVATE gtest::gtest multithreaded mvplayer)
