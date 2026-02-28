add_library(GearBoxFileSim STATIC
    FileAudioIO.cpp
    stb_vorbis_impl.c
)

target_include_directories(GearBoxFileSim
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(GearBoxFileSim
    PUBLIC  GearBoxDSP
    PRIVATE dr_libs
    PRIVATE stb_libs
    PRIVATE portaudio_static
    PRIVATE spdlog::spdlog
)

if(MSVC)
    target_compile_options(GearBoxFileSim PRIVATE /W4)
else()
    target_compile_options(GearBoxFileSim PRIVATE -Wall -Wextra)
endif()
