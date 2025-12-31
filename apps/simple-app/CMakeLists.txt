add_executable(simple_app)

target_sources(simple_app PRIVATE main.cpp)
target_link_libraries(simple_app PRIVATE multithreaded)
