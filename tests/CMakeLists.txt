include(GoogleTest)

add_executable(gearboxfx_tests
    test_effect_chain.cpp
    test_effects.cpp
    test_preset_store.cpp
)

target_link_libraries(gearboxfx_tests
    PRIVATE GearBoxDSP
    PRIVATE GTest::gtest_main
    PRIVATE spdlog::spdlog
)

target_include_directories(gearboxfx_tests
    PRIVATE ${CMAKE_SOURCE_DIR}/dsp-core/include
)

gtest_discover_tests(gearboxfx_tests)

# Copy test presets
add_custom_command(TARGET gearboxfx_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/presets
        $<TARGET_FILE_DIR:gearboxfx_tests>/presets
    COMMENT "Copying presets for tests"
)
