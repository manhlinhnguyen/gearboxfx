add_library(GearBoxDSP STATIC
    src/EffectNode.cpp
    src/EffectEngine.cpp
    src/EffectChain.cpp
    src/ParameterManager.cpp
    src/PresetStore.cpp
    src/EffectNodeRegistry.cpp
    src/effects/dynamics/NoiseGateNode.cpp
    src/effects/dynamics/CompressorNode.cpp
    src/effects/gain/CleanBoostNode.cpp
    src/effects/gain/OverdriveNode.cpp
    src/effects/gain/DistortionNode.cpp
    src/effects/modulation/ChorusNode.cpp
    src/effects/modulation/TremoloNode.cpp
    src/effects/time/DelayNode.cpp
    src/effects/time/ReverbNode.cpp
)

target_include_directories(GearBoxDSP
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GearBoxDSP
    PUBLIC  nlohmann_json::nlohmann_json
    PRIVATE spdlog::spdlog
)

if(MSVC)
    target_compile_options(GearBoxDSP PRIVATE /W4)
else()
    target_compile_options(GearBoxDSP PRIVATE -Wall -Wextra -Wpedantic)
endif()
