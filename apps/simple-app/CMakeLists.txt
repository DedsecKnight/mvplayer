add_executable(simple_app)

target_sources(simple_app PRIVATE main.cpp)
target_link_libraries(simple_app PRIVATE multithreaded)

set(RUNTIME_DIRS "$<TARGET_RUNTIME_DLLS:simple_app>")

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_custom_command(TARGET simple_app POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:simple_app>
            $<TARGET_FILE_DIR:simple_app>
        COMMAND_EXPAND_LISTS
    )
endif()
