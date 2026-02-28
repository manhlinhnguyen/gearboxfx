# ── imgui static library (GLFW + OpenGL3 backends + imgui-knobs) ────────────
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    ${imgui_knobs_SOURCE_DIR}/imgui-knobs.cpp
)

target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_knobs_SOURCE_DIR}
)

target_link_libraries(imgui_lib PUBLIC glfw opengl32)

# Suppress warnings from third-party code
if(MSVC)
    target_compile_options(imgui_lib PRIVATE /W0)
else()
    target_compile_options(imgui_lib PRIVATE -w)
endif()

# ── GearBoxDesktopGui static library ────────────────────────────────────────
add_library(GearBoxDesktopGui STATIC
    GearBoxApp.cpp
    GuiAudioIO.cpp
    panels/TransportPanel.cpp
    panels/PresetPanel.cpp
    panels/ChainPanel.cpp
    panels/ParamPanel.cpp
)

target_include_directories(GearBoxDesktopGui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(GearBoxDesktopGui
    PUBLIC  GearBoxDSP imgui_lib
    PRIVATE portaudio_static spdlog::spdlog dr_libs
)

if(MSVC)
    target_compile_options(GearBoxDesktopGui PRIVATE /W4)
else()
    target_compile_options(GearBoxDesktopGui PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
