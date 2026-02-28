# GearBox FX

> **Your Sound. Your Code.**

GearBox FX is a programmable multi-effects guitar pedal with an open-source preset ecosystem, JSON-based preset format, and a cross-platform DSP core written in pure C++17.

---

## Features

- 14 built-in effects (dynamics, EQ, gain, modulation, output, time-based)
- JSON preset format — human-readable, version-controlled, shareable
- Phase 1a: WAV file pipeline with CLI
- Phase 1b: Real-time desktop GUI — horizontal block view, inline knobs, file browser, loop playback
- Pure C++17 DSP core — no platform code, portable to embedded targets
- Planned: STM32H743 hardware, mobile app (Flutter), cloud backend (NestJS)

---

## Development Phases

| Phase | Status | Description |
|-------|--------|-------------|
| **1a** | Done (2026-02-28) | File audio pipeline — WAV→DSP→WAV, 9 effects, CLI, 7 presets |
| **1b** | Done (2026-02-28) | Dear ImGui desktop GUI — horizontal block view, inline knobs, file browser, loop, 14 effects, 11 presets |
| **2** | Q2 2026 | Alpha hardware — STM32H743 + PCB Rev A |
| **3** | Q3 2026 | Beta — cloud backend, IR Loader, Looper, AmpSim |
| **4** | Q4 2026 | Production release |

---

## Repository Structure

```
gearboxfx/
├── dsp-core/                 # Pure C++17 DSP engine (platform-agnostic)
│   ├── include/
│   │   ├── effects/
│   │   │   ├── dynamics/     # NoiseGateNode, CompressorNode
│   │   │   ├── gain/         # CleanBoostNode, OverdriveNode, DistortionNode
│   │   │   ├── modulation/   # ChorusNode, TremoloNode
│   │   │   └── time/         # DelayNode, ReverbNode
│   │   └── ...               # EffectEngine, EffectChain, PresetStore, ...
│   └── src/
├── platform/
│   ├── file-sim/             # Phase 1a — FileAudioIO (WAV I/O)
│   └── desktop-gui/          # Phase 1b — GuiAudioIO + ImGui panels
│       └── panels/           # TransportPanel, PresetPanel, ChainPanel (block view)
├── app/                      # Phase 1a CLI executable
├── app-gui/                  # Phase 1b desktop GUI executable
├── presets/                  # JSON preset files
├── test-audio/               # WAV input samples
├── tests/                    # GoogleTest unit tests
├── firmware/                 # (Phase 2) STM32 + ESP32 firmware stubs
├── mobile-app/               # (Phase 3) Flutter mobile app stub
├── cloud-backend/            # (Phase 3) NestJS backend stub
├── documents/                # Architecture documents (Vietnamese)
├── CMakeLists.txt
└── CMakePresets.json
```

---

## Signal Flow

```
[WAV / Mic Input]
        │
        ▼
  IAudioIO / GuiAudioIO       ← platform layer
        │
        ▼
   EffectEngine               ← bypass, volume, preset name
        │
        ▼
   EffectChain                ← ordered effect nodes, ping-pong buffer
        │
   ┌────┴──────────────────┐
   │  Effect Node 1        │
   │  Effect Node 2        │
   │  ...                  │
   └───────────────────────┘
        │
        ▼
  [WAV / Speaker Output]
```

---

## Built-in Effects (14 total)

| Type ID | Effect | Key Parameters |
|---------|--------|----------------|
| `dynamics.noise_gate` | Noise Gate | threshold_db, attack_ms, release_ms |
| `dynamics.compressor` | Compressor | threshold_db, ratio, attack_ms, release_ms, makeup_db, knee_db |
| `eq.parametric` | Parametric EQ | bass_db, mid_db, mid_freq, treble_db |
| `gain.clean_boost` | Clean Boost | gain_db |
| `gain.overdrive` | Overdrive | gain, tone, level |
| `gain.distortion` | Distortion | gain, tone, level, asymmetry |
| `modulation.chorus` | Chorus | rate, depth, mix, voices |
| `modulation.flanger` | Flanger | rate, depth, feedback, mix |
| `modulation.phaser` | Phaser | rate, depth, feedback, mix |
| `modulation.pitch_shifter` | Pitch Shifter | semitones, mix |
| `modulation.tremolo` | Tremolo | rate, depth, waveform |
| `output.volume` | Volume + Limiter | volume_db, limiter_threshold_db |
| `time.delay` | Delay | time_ms, feedback, mix, bpm_sync, bpm |
| `time.reverb` | Reverb | size, decay, damping, pre_delay_ms, mix |

---

## Included Presets (11 total)

| File | Name | Description |
|------|------|-------------|
| `01_clean_boost.json` | Clean Boost | Noise gate + +6dB boost |
| `02_mild_overdrive.json` | Mild Overdrive | Light gain, classic tone |
| `03_heavy_distortion.json` | Heavy Distortion | High-gain distortion |
| `04_chorus_clean.json` | Chorus Clean | Clean tone with chorus |
| `05_delay_reverb.json` | Delay + Reverb | Ambient delay and reverb |
| `06_cathedral_shimmer.json` | Cathedral Shimmer | Large reverb, long decay |
| `08_bright_clean_eq.json` | Bright Clean EQ | Parametric EQ, bright presence boost |
| `09_jet_flanger.json` | Jet Flanger | Classic jet flanger with feedback |
| `10_phaser_funk.json` | Phaser Funk | Funky slow phaser sweep |
| `11_octave_up.json` | Octave Up | +12 semitone pitch shifter |
| `12_warm_overdrive_full.json` | Warm Overdrive Full | Full chain: gate → comp → EQ → overdrive → reverb |

---

## Preset JSON Format

```json
{
  "preset_id": "uuid-v4",
  "format_version": "1.0",
  "name": "My Preset",
  "routing_mode": "serial",
  "effect_chain": [
    {
      "id": "ng_1",
      "type": "dynamics.noise_gate",
      "enabled": true,
      "params": {
        "threshold_db": -70.0,
        "attack_ms": 5.0,
        "release_ms": 150.0
      }
    }
  ],
  "output_eq": { "bass_db": 0.0, "mid_db": 0.0, "treble_db": 0.0 },
  "output_volume": 0.85
}
```

---

## Build Requirements

- CMake 3.21+
- MinGW GCC (C++17) — `C:\ProgramData\mingw64\mingw64\bin\`
- Internet connection (first build — FetchContent downloads ~100MB)

### Dependencies (auto-fetched via FetchContent)

| Library | Version | Use |
|---------|---------|-----|
| nlohmann/json | v3.11.3 | JSON preset parsing |
| googletest | v1.14.0 | Unit tests |
| spdlog | v1.14.1 | Logging |
| dr_libs | master | WAV decode/encode |
| portaudio | v19.7.0 | Real-time audio I/O |
| glfw | 3.4 | Window + OpenGL context |
| imgui | v1.91.6 | Immediate-mode GUI |
| imgui-knobs | main | Rotary knob widgets |

---

## Building (Windows + PowerShell)

```powershell
cd C:\codes\gearboxfx

$env:CC  = "C:\ProgramData\mingw64\mingw64\bin\gcc.exe"
$env:CXX = "C:\ProgramData\mingw64\mingw64\bin\g++.exe"

# Configure
& "C:\Program Files\CMake\bin\cmake.exe" -B build --preset file-sim-release

# Build
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release
```

### Run — Phase 1a CLI

```powershell
# Process WAV file → output WAV
.\build\app\gearboxfx.exe --input test-audio\guitar.wav `
    --preset presets\02_mild_overdrive.json --output out.wav

# Stream to speakers
.\build\app\gearboxfx.exe --input test-audio\guitar.wav `
    --preset presets\04_chorus_clean.json --play

# List registered effects
.\build\app\gearboxfx.exe --list-presets
```

### Run — Phase 1b Desktop GUI

```powershell
.\build\app-gui\gearboxfx-gui.exe
```

The GUI opens a 1280×720 window with a 3-panel layout:
- **Transport** (top-left): Browse WAV file, Play/Stop/Rewind, Loop toggle, volume slider, VU meter
- **Presets** (top-right of transport): load/save presets from the `presets/` directory
- **Effects** (full bottom, scrollable): horizontal block view — each effect is a block with a category-colored header, inline knobs (3 per row), ON/OFF toggle, and reorder/delete buttons

### Run Tests

```powershell
& "C:\Program Files\CMake\bin\ctest.exe" --test-dir build --output-on-failure
```

### Clean Rebuild

```powershell
Remove-Item -Recurse -Force build
$env:CC  = "C:\ProgramData\mingw64\mingw64\bin\gcc.exe"
$env:CXX = "C:\ProgramData\mingw64\mingw64\bin\g++.exe"
& "C:\Program Files\CMake\bin\cmake.exe" -B build --preset file-sim-release
& "C:\Program Files\CMake\bin\cmake.exe" --build build --config Release
```

---

## Adding a New Effect

1. Create `MyEffectNode.h` in `dsp-core/include/effects/<category>/`
2. Create `MyEffectNode.cpp` in `dsp-core/src/effects/<category>/`
3. Inherit from `EffectNode`, call `registerParam()` in constructor, implement `process()`
4. Add one line to `EffectNodeRegistry::registerAll()`:
   ```cpp
   reg<MyEffectNode>("category.my_effect");
   ```
5. Add the `.cpp` to `dsp-core/CMakeLists.txt`

The GUI picks it up automatically in the "Add Effect" popup — no other files need to change.

---

## Architecture Notes

- **Core invariant**: `dsp-core/` never contains platform-specific code. All hardware differences are isolated behind `IAudioIO`.
- **Thread safety**: `ParameterManager` is mutex-guarded — safe to call `setParam()` from any thread. Chain modifications use `GuiAudioIO::lockChain()` with `try_lock` in the audio callback (one silent block if contended).
- **Preset loading**: `PresetStore::loadFromFile()` → `EffectNodeRegistry::create()` — fully dynamic, no recompile needed for new presets.

---

## Audio Performance Targets

| Metric | Target |
|--------|--------|
| Latency (laptop) | < 15ms |
| Latency (STM32) | < 10ms |
| THD+N | < 0.01% |
| Noise floor | < -95 dBFS |
| Buffer size | 64–256 samples |
| Sample rate | 48 kHz / 24-bit |

---

## Hardware Architecture (Phase 2+)

- **STM32H743** (DSP MCU, 480MHz Cortex-M7): runs `dsp-core`, audio I/O via SAI/I2S DMA
- **ESP32-S3** (Control MCU): BLE 5.0 GATT server, OLED display, footswitches, MIDI TRS
- Communication: UART frame protocol (0xAA55 start, CRC16)

---

## License

Open source. See LICENSE file for details.
