find_package(CLI11 REQUIRED)

add_executable(demo)

target_sources(demo PRIVATE main.cpp)
target_link_libraries(demo PRIVATE mvplayer CLI11::CLI11)
