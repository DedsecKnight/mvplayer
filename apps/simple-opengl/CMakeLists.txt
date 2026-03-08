add_executable(simple_opengl)

target_sources(simple_opengl PRIVATE main.cpp)
target_link_libraries(simple_opengl PRIVATE opengl SDL3::SDL3 stb::stb)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(RUNTIME_DIRS "$<TARGET_RUNTIME_DLLS:simple_opengl>")
    add_custom_command(TARGET simple_opengl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:simple_opengl>
            $<TARGET_FILE_DIR:simple_opengl>
        COMMAND_EXPAND_LISTS
    )
endif()
