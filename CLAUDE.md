# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**GearBox FX** is a programmable multi-effects guitar pedal — open-source preset ecosystem, JSON-based presets, cross-platform DSP core. Tagline: *"Your Sound. Your Code."*

This repository is a **monorepo**. **Phase 1a and Phase 1b are both fully implemented.** The documents here (in Vietnamese) define the full architecture.

---

## Build Commands

> **Environment (Windows + MinGW, PowerShell):** `cmake` và `gcc` không có trong PATH. Dùng lệnh PowerShell bên dưới với `$env:` và toán tử `&`.

### Phase 1a + 1b — Full build (CLI + Desktop GUI)
Requires: CMake 3.21+, MinGW GCC C++17. All deps fetched automatically via FetchContent.

```powershell
cd C:\codes\gearboxfx

# Configure
$env:CC  = "C:\ProgramData\mingw64\mingw64\bin\gcc.exe"
$env:CXX = "C:\ProgramData\mingw64\mingw64\bin\g++.exe"
& "C:\Program Files\CMake\bin\cmake.exe" -B build --preset file-sim-release

# Build
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release
```

```powershell
# Phase 1a CLI — process WAV → output WAV
.\build\app\gearboxfx.exe --input test-audio\guitar.wav `
    --preset presets\02_mild_overdrive.json --output out.wav

# Phase 1a CLI — stream to speakers
.\build\app\gearboxfx.exe --input test-audio\guitar.wav `
    --preset presets\04_chorus_clean.json --play

# Phase 1a CLI — list registered effects
.\build\app\gearboxfx.exe --list-presets

# Phase 1b GUI — desktop application (1280×720, real-time)
.\build\app-gui\gearboxfx-gui.exe

# Run unit tests
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build --output-on-failure
```

CMake presets: `file-sim-debug`, `file-sim-release` (see `CMakePresets.json`).

### Phase 1b first build note
First configure after Phase 1b downloads ~100MB extra (GLFW, Dear ImGui, imgui-knobs). Cached in `build/_deps/` for subsequent builds.

### Clean rebuild (PowerShell)
```powershell
Remove-Item -Recurse -Force build
$env:CC  = "C:\ProgramData\mingw64\mingw64\bin\gcc.exe"
$env:CXX = "C:\ProgramData\mingw64\mingw64\bin\g++.exe"
& "C:\Program Files\CMake\bin\cmake.exe" -B build --preset file-sim-release
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release
```

### STM32 Firmware (not yet implemented, requires arm-none-eabi-gcc)
```bash
cd firmware
cmake -B build-stm32 -DPLATFORM=stm32 \
      -DCMAKE_TOOLCHAIN_FILE=platform/stm32/toolchain-stm32h7.cmake
cmake --build build-stm32
```

### ESP32 Firmware (not yet implemented, requires PlatformIO)
```bash
cd firmware/control
pio run --target upload
```

### Mobile App (not yet implemented, requires Flutter 3.x)
```bash
cd mobile-app
flutter pub get
flutter run
```

### Cloud Backend (not yet implemented, requires Docker)
```bash
cd cloud-backend
docker-compose up -d
```

### Tests
- C++ (DSP Core + GUI): GoogleTest — `ctest` in the CMake build directory
- Backend (future): `jest` inside `cloud-backend/services/preset-api/`
- Flutter (future): `flutter test` inside `mobile-app/`

### FetchContent Dependencies
| Library | Version | Phase | Use |
|---|---|---|---|
| `nlohmann/json` | v3.11.3 | 1a | JSON preset parse/save |
| `googletest` | v1.14.0 | 1a | Unit tests |
| `spdlog` | v1.14.1 | 1a | Logging |
| `dr_libs` | master | 1a | WAV decode/encode (header-only) |
| `portaudio` | v19.7.0 | 1a | Real-time audio I/O |
| `glfw` | 3.4 | 1b | Window + OpenGL context |
| `imgui` | v1.91.6 | 1b | Immediate-mode GUI |
| `imgui-knobs` | main | 1b | Rotary knob widgets |

---

## Architecture

### The Core Invariant
The `dsp-core/` module is **pure C++17 and never changes between platforms**. All hardware differences are isolated behind `IAudioIO` (Phase 1a) or behind platform-specific `GuiAudioIO` (Phase 1b). When adding effects or preset logic, **never introduce platform-specific code into `dsp-core/`**.

### Signal Flow
```
[Input] → IAudioIO/GuiAudioIO → EffectEngine → EffectChain → [Effect Nodes] → [Output]
```

### Platform Abstraction
| Platform | Class | Location | Status |
|---|---|---|---|
| Phase 1a CLI | `FileAudioIO` | `platform/file-sim/` | **Implemented** |
| Phase 1b GUI | `GuiAudioIO` | `platform/desktop-gui/` | **Implemented** |
| STM32H743 (Phase 2) | `STM32AudioIO` | `platform/stm32/` | Planned |
| Bela (fallback) | `BelaAudioIO` | `platform/bela/` | Planned |

### DSP Core Components (`dsp-core/`)
| Component | Role |
|---|---|
| `IAudioIO` | HAL interface — only entry point to platform |
| `EffectEngine` | Top-level orchestrator; owns bypass, EffectChain, ParameterManager |
| `EffectChain` | Ordered list of nodes; ping-pong buffer between nodes |
| `EffectNode` | Base class for all effects; implement `process(input, output, numSamples)` |
| `EffectNodeRegistry` | Factory — register a new effect with one `reg<T>("category.name")` line |
| `ParameterManager` | Thread-safe (mutex-guarded) parameter store; safe for real-time audio thread |
| `PresetStore` | Load/save JSON presets via `nlohmann/json`; maps to file system or Flash |
| `TunerProcessor` | YIN pitch detection; runs in parallel, does not affect audio path (Phase 1b+) |
| `LooperProcessor` | Double-buffer circular recorder (Phase 1b+) |

### EffectEngine Public API (relevant additions in Phase 1b)
```cpp
void  setOutputVolume(float v);   // set scalar volume after chain
float outputVolume()    const;    // read current output volume
void  setPresetName(const std::string& n); // update in-memory preset name
const std::string& presetName() const;
```

### Adding a New Effect
1. Create `SomeNode.h` in `dsp-core/include/effects/<category>/`
2. Create `SomeNode.cpp` in `dsp-core/src/effects/<category>/`
3. Inherit from `EffectNode`, call `registerParam()` in constructor, implement `process()`
4. Add one line to `EffectNodeRegistry::registerAll()` in `dsp-core/src/EffectNodeRegistry.cpp`: `reg<SomeNode>("category.some_name");`
5. Add the `.cpp` to `dsp-core/CMakeLists.txt` source list
6. No other files need to change — GUI picks it up automatically in the "Add Effect" popup

### Registered Effect Type IDs (Phase 1a)
| Type ID | Node | Key Params |
|---|---|---|
| `dynamics.noise_gate` | NoiseGateNode | threshold_db, attack_ms, release_ms |
| `dynamics.compressor` | CompressorNode | threshold_db, ratio, attack_ms, release_ms, makeup_db, knee_db |
| `gain.clean_boost` | CleanBoostNode | gain_db |
| `gain.overdrive` | OverdriveNode | gain, tone, level |
| `gain.distortion` | DistortionNode | gain, tone, level, asymmetry |
| `modulation.chorus` | ChorusNode | rate, depth, mix, voices |
| `modulation.tremolo` | TremoloNode | rate, depth, waveform |
| `time.delay` | DelayNode | time_ms, feedback, mix, bpm_sync, bpm |
| `time.reverb` | ReverbNode | size, decay, damping, pre_delay_ms, mix |

### Preset JSON Schema
```json
{
  "preset_id": "uuid-v4",
  "format_version": "1.0",
  "name": "...",
  "routing_mode": "serial",
  "effect_chain": [
    { "id": "ng_1", "type": "dynamics.noise_gate", "enabled": true, "params": { ... } }
  ],
  "expression_pedal": { "target_effect_id": "...", "target_param": "...", "range_min": 0.0, "range_max": 1.0, "curve": "linear" },
  "output_eq": { "bass_db": 0.0, "mid_db": 0.0, "treble_db": 0.0 },
  "output_volume": 0.85
}
```

---

## Phase 1b — Desktop GUI

### Structure
```
platform/desktop-gui/
├── CMakeLists.txt          imgui_lib (static) + GearBoxDesktopGui (static)
├── AppContext.h            shared state struct passed to all panels each frame
├── GearBoxApp.h / .cpp     GLFW window, ImGui docking, render loop
├── GuiAudioIO.h / .cpp     PortAudio real-time WAV playback + engine bridge
└── panels/
    ├── TransportPanel      WAV load, play/stop, progress bar, VU meter
    ├── PresetPanel         filesystem scan, selectable list, save-as, new
    ├── ChainPanel          add/remove/move/enable nodes, "Add Effect" popup
    └── ParamPanel          knobs (range ≤ 2.0) + sliders (wide range)
app-gui/
├── CMakeLists.txt          gearboxfx-gui exe, post-build copies presets/
└── main_gui.cpp            10-line entry point
```

### GUI Threading Model
| Thread | Responsibility |
|---|---|
| Main (GLFW, ~60fps) | ImGui render + event poll |
| Audio (PortAudio, ~5ms) | `GuiAudioIO::doCallback()` — de-interleave → `engine.processBlock()` → re-interleave |

**Param changes** (`engine.setParam()`) are safe from any thread via `ParameterManager` mutex.

**Chain modifications** (add/remove/move nodes): GUI acquires `GuiAudioIO::lockChain()` → returns `std::unique_lock<std::mutex>`; audio callback uses `try_to_lock` — outputs one block of silence if lock unavailable (inaudible).

### AppContext
```cpp
struct AppContext {
    EffectEngine* engine;
    GuiAudioIO*   audio;
    std::string*  selectedEffectId;  // ID of effect selected in ChainPanel
    std::string   presetsDir;
    double        sampleRate;
    int           blockSize;
};
```
Created fresh in `GearBoxApp::render()` each frame from `m_*` members; mutable fields (e.g. `sampleRate`) synced back after render.

### ParamPanel Widget Rules
- `(def.maxValue - def.minValue) <= 2.0` → `ImGuiKnobs::Knob` (Wiper variant), rendered side-by-side
- Otherwise → `ImGui::SliderFloat`, one per line

### Key Build Fix (imgui FetchContent)
Dear ImGui and imgui-knobs have no `CMakeLists.txt` at their repo root. Use `FetchContent_Populate` (not `FetchContent_MakeAvailable`) to download sources only, then compile them manually in `platform/desktop-gui/CMakeLists.txt`.

`DR_WAV_IMPLEMENTATION` is defined in `GuiAudioIO.cpp`. `gearboxfx-gui` does **not** link `GearBoxFileSim`, so there is no duplicate-symbol conflict.

---

## Known Build Fixes (applied, do not revert)

1. `CMakeLists.txt` `LANGUAGES C CXX` (was `CXX` only) — fixes `CMAKE_C_COMPILE_OBJECT` error with PortAudio under CMake 4.x
2. `platform/file-sim/CMakeLists.txt`: links `portaudio_static` (not `portaudio`) — `PA_BUILD_SHARED=OFF` means only the static target exists
3. `CMakeLists.txt`: `add_link_options(-static-libgcc -static-libstdc++)` for MINGW — fixes `0xc0000139` DLL-not-found error when `gtest_discover_tests` runs executables during build
4. `tests/test_effects.cpp`: `TEST_SILENCE_PASSTHROUGH(TestName, TypeStr)` 2-arg macro — dotted type IDs like `dynamics.noise_gate` are not valid C++ identifiers

---

## Dual-MCU Hardware Architecture (Phase 2+)

- **STM32H743** (DSP MCU, 480MHz Cortex-M7): runs `dsp-core`, audio I/O via SAI/I2S DMA, FatFS on MicroSD for presets
- **ESP32-S3** (Control MCU): BLE 5.0 GATT server, OLED display, footswitches, MIDI TRS, communicates with STM32 via UART frame protocol (0xAA55 start, CRC16)

### BLE GATT Service (UUID: 0xFB00)
| Characteristic | UUID | Direction | Use |
|---|---|---|---|
| PRESET_DATA | 0xFB01 | Write/Notify | Load preset JSON (chunked) |
| PRESET_SELECT | 0xFB02 | Write/Notify | Select preset by index/UUID |
| PARAM_CONTROL | 0xFB03 | Write (no-response) | Real-time param: [effect_id, param_id, float32] |
| DEVICE_STATUS | 0xFB04 | Read/Notify | Status JSON |
| TUNER_DATA | 0xFB05 | Notify (10Hz) | [freq:f32, note:u8, cents:i8] |
| OTA_FIRMWARE | 0xFB06 | Write | Firmware binary (ESP32 OTA) |

### Mobile App (Flutter/Dart, Phase 3+)
Clean Architecture + Riverpod 2 state management. Structure: `lib/core/` (models, repositories, services), `lib/features/` (device, preset_editor, effect_library, community, settings), `lib/shared/widgets/`.

### Cloud Backend (NestJS + PostgreSQL, Phase 3+)
REST API at `https://api.gearboxfx.io/v1`. Key resources: `/auth`, `/presets`, `/users/me/presets`, `/firmware`, `/ir-files`. Preset data stored as `JSONB` column. GIN index on `tags[]`.

---

## Development Phases

| Phase | Status | Goal |
|---|---|---|
| **1a** | **Complete** (2026-02-28) | File audio pipeline (WAV→DSP→WAV), 9 effects, CLI, 7 presets |
| **1b** | **Complete** (2026-02-28) | Dear ImGui desktop GUI — real-time WAV playback, preset/chain/param editing |
| **2** | Q2 2026 | Alpha hardware (STM32H743 + PCB Rev A) |
| **3** | Q3 2026 | Beta — cloud backend, advanced features (IR Loader, Looper, AmpSim) |
| **4** | Q4 2026 | Production release |

## Audio Performance Targets
- End-to-end latency: < 10ms on STM32, < 15ms on laptop
- THD+N: < 0.01% | Noise floor: < -95 dBFS (Phase 4)
- Buffer size: 64–256 samples
- Sample rate: 48 kHz / 24-bit

## Hardware Fallback Options
If STM32H743 is unavailable: Teensy 4.1 (IMXRT1062, 600MHz) or Bela Board.
