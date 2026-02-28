add_executable(gearboxfx-gui ${CMAKE_CURRENT_SOURCE_DIR}/main_gui.cpp)

target_link_libraries(gearboxfx-gui PRIVATE GearBoxDesktopGui)

# Copy presets/ next to the executable after each build
add_custom_command(TARGET gearboxfx-gui POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/presets
        $<TARGET_FILE_DIR:gearboxfx-gui>/presets
    COMMENT "Copying presets to output directory"
)

# On Windows/MinGW, statically bundle the runtime (already set at root level
# via add_link_options, so no duplication needed here).
