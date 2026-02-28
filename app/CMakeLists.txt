add_executable(gearboxfx main.cpp)

target_link_libraries(gearboxfx
    PRIVATE GearBoxFileSim
    PRIVATE GearBoxDSP
    PRIVATE spdlog::spdlog
)

if(MSVC)
    target_compile_options(gearboxfx PRIVATE /W4)
else()
    target_compile_options(gearboxfx PRIVATE -Wall -Wextra)
endif()

# Copy presets to build directory
add_custom_command(TARGET gearboxfx POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/presets
        $<TARGET_FILE_DIR:gearboxfx>/presets
    COMMENT "Copying presets to build directory"
)
