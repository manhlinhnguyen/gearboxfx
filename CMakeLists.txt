cmake_minimum_required(VERSION 3.21)
project(GearBoxFX VERSION 0.1.0 LANGUAGES C CXX)

# Allow FetchContent subprojects (e.g. PortAudio v19.7.0) that still use
# cmake_minimum_required < 3.5 to configure under CMake 4.x.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Statically bundle MinGW runtime so executables run without DLLs in PATH
if(MINGW)
    add_link_options(-static-libgcc -static-libstdc++)
endif()

# ── FetchContent dependencies ──────────────────────────────────────────────────
include(FetchContent)

# nlohmann/json (header-only)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# GoogleTest
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)

# spdlog
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
)

# dr_libs (header-only, WAV decode/encode)
FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# PortAudio
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG        v19.7.0
    GIT_SHALLOW    TRUE
)

# GLFW (for desktop GUI)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
    GIT_SHALLOW    TRUE
)

# Dear ImGui (header+source, no root CMakeLists.txt — compiled manually)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.6
    GIT_SHALLOW    TRUE
)

# imgui-knobs (header+source, no root CMakeLists.txt)
FetchContent_Declare(
    imgui_knobs
    GIT_REPOSITORY https://github.com/altschuler/imgui-knobs.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)

# stb (single-file libraries — stb_vorbis.c for OGG decode, no CMakeLists.txt)
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(PA_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(PA_BUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(PA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(nlohmann_json googletest spdlog dr_libs portaudio glfw)

# imgui and imgui-knobs have no CMakeLists.txt — just download sources
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

FetchContent_GetProperties(imgui_knobs)
if(NOT imgui_knobs_POPULATED)
    FetchContent_Populate(imgui_knobs)
endif()

FetchContent_GetProperties(stb)
if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
endif()

# Create interface target for dr_libs (header-only)
add_library(dr_libs INTERFACE)
target_include_directories(dr_libs INTERFACE ${dr_libs_SOURCE_DIR})

# Interface target for stb (consumers #include "stb_vorbis.c" directly)
add_library(stb_libs INTERFACE)
target_include_directories(stb_libs INTERFACE ${stb_SOURCE_DIR})

# ── Subdirectories ─────────────────────────────────────────────────────────────
add_subdirectory(dsp-core)
add_subdirectory(platform/file-sim)
add_subdirectory(platform/desktop-gui)
add_subdirectory(app)
add_subdirectory(app-gui)

enable_testing()
add_subdirectory(tests)
