# GearBox FX — Phase 1b: Desktop GUI

**Status:** Implemented (2026-02-28)
**Goal:** Real-time desktop GUI — load WAV, play through effects, edit presets/params live.

---

## What Was Built

A fully functional Dear ImGui desktop application layered on top of the existing Phase 1a DSP core. Zero changes to `dsp-core/` beyond two inline accessors added to `EffectEngine`.

### New Files

```
platform/desktop-gui/
├── CMakeLists.txt          — imgui_lib + GearBoxDesktopGui static libs
├── AppContext.h            — shared state struct passed to all panels
├── GearBoxApp.h / .cpp     — GLFW window, ImGui docking, render loop
├── GuiAudioIO.h / .cpp     — PortAudio real-time WAV playback + engine bridge
└── panels/
    ├── TransportPanel.h/cpp  — WAV load, play/stop, progress, VU meter
    ├── PresetPanel.h/cpp     — preset list, load, save-as, new
    ├── ChainPanel.h/cpp      — add/remove/move/enable effect nodes
    └── ParamPanel.h/cpp      — knobs (normalized params) + sliders (wide range)

app-gui/
├── CMakeLists.txt          — gearboxfx-gui executable, copies presets/
└── main_gui.cpp            — entry point (10 lines)
```

### Modified Files

| File | Change |
|------|--------|
| `CMakeLists.txt` | FetchContent: glfw 3.4, imgui v1.91.6, imgui-knobs; add subdirs |
| `dsp-core/include/EffectEngine.h` | `setOutputVolume()`, `outputVolume()`, `setPresetName()`, `presetName()` |

---

## Architecture

```
GearBoxApp (GLFW 1280×720, ImGui docking, ~60fps)
    │
    ├── AppContext { engine*, audio*, selectedEffectId*, presetsDir, sr, bs }
    │       passed by value to each panel's render() every frame
    │
    ├── GuiAudioIO          — PortAudio stream (continuous, non-blocking)
    │       │  paCallback → doCallback (audio thread)
    │       │      try_lock(m_chainMutex) — skip block if GUI modifying chain
    │       │      de-interleave → engine.processBlock() → re-interleave
    │       │      atomic: m_readPos, m_playing, m_outputLevel
    │       └── loadFile() → dr_wav decode → m_decoded (interleaved float32)
    │
    ├── TransportPanel      — WAV path input, Play/Stop, progress bar, VU meter
    ├── PresetPanel         — filesystem scan, selectable list, save-as
    ├── ChainPanel          — node list with enable/move/remove, add popup
    └── ParamPanel          — knobs (range ≤ 2.0) + sliders (wide range)
```

### Threading Model

| Thread | Work |
|--------|------|
| Main (GLFW) | ImGui render + event poll at ~60fps |
| Audio (PortAudio) | `doCallback()` at buffer rate (block=256, 48kHz ≈ 5.3ms) |

**Sync:** `ParameterManager::set()` is mutex-guarded (safe from any thread).
**Chain modifications** (add/remove/move nodes): GUI calls `audio.lockChain()`, which acquires `m_chainMutex`; audio callback uses `try_to_lock` and outputs silence for that block if the lock is taken. Chain modifications are infrequent (button clicks) so this is transparent.

---

## Build Instructions

```bash
# First time (downloads glfw + imgui + imgui-knobs, ~100 MB extra)
cd /c/codes/gearboxfx
GCC=/c/ProgramData/mingw64/mingw64/bin
CC=$GCC/gcc.exe CXX=$GCC/g++.exe \
  /c/Program\ Files/CMake/bin/cmake.exe -B build --preset file-sim-release
/c/Program\ Files/CMake/bin/cmake.exe --build build --config Release

# Run the GUI
./build/app-gui/gearboxfx-gui.exe

# CLI tool still works
./build/app/gearboxfx.exe --input test-audio/guitar.wav \
    --preset presets/04_chorus_clean.json --output out.wav

# Unit tests still pass
cd build && /c/Program\ Files/CMake/bin/ctest.exe --output-on-failure
```

---

## UI Layout

```
┌─────────────────────────────────────────────────────────────────┐
│ GearBox FX                                          [_][□][X]   │
├──────────────────┬──────────────────┬───────────────────────────┤
│ TRANSPORT        │ PRESETS          │ EFFECT CHAIN              │
│                  │                  │                           │
│ WAV: kawai.wav   │ 01_clean_boost   │ ☑ chorus_1  mod.chorus   │
│ [▶ Play][■ Stop] │ ▶ 04_chorus... ←│ ☑ reverb_1  time.reverb  │
│ ████████░░░ 65%  │ 05_delay_reverb  │ [+ Add Effect]           │
│ Vol [=====|==]   │                  │                           │
│ VU ▓▓▓▓░░░░░░░░  │ [+New][Save As]  │                           │
├──────────────────┴──────────────────┴───────────────────────────┤
│ PARAMETERS  —  chorus_1  (modulation.chorus)                    │
│  (rate)    (depth)    (mix)    (voices)                         │
│   0.35      0.85       0.65     4.0                             │
└─────────────────────────────────────────────────────────────────┘
```

### Panel Descriptions

**Transport**
- InputText for WAV path + `[Load]` button → decodes WAV, prepares engine
- `[Play]` / `[Stop]` / `[Rewind]` buttons
- `ProgressBar` showing playback position [0–1]
- `SliderFloat` for output volume [0–1.5]
- Custom VU meter (draw list) with green/yellow/red zones

**Presets**
- Scans `presets/*.json` via `std::filesystem`; auto-scans on first render
- Scrollable `Selectable` list; click → `engine.loadPreset()` + `chain.prepare()`
- `[+ New]` → `chain.clear()`, blank slate
- InputText + `[Save]` → `engine.savePreset(presetsDir/name.json)` + re-scan

**Effect Chain**
- Iterates `engine.chain().nodes()` — checkbox (enable), selectable (select for params)
- `[^]` / `[v]` — move via `removeNode` + `insertNode` under `lockChain()`
- `[X]` — remove node, clears `selectedEffectId` if needed
- `[+ Add Effect]` popup — sorted `registeredTypes()` list → `registry.create()` → unique ID → `chain.addNode()`

**Parameters**
- Shows params for `selectedEffectId`
- Iterates `paramDefs()` sorted alphabetically
- **Knob** (`ImGuiKnobs::Knob`, `ImGuiKnobVariant_Wiper`): params with `max − min ≤ 2.0` (gain, mix, depth, etc.) — rendered side-by-side with `SameLine()`
- **Slider** (`SliderFloat`): wide-range params (time_ms, threshold_db, bpm, etc.)
- All changes → `engine.setParam("effectId.paramName", value)` — thread-safe via ParameterManager

---

## GuiAudioIO Design

```cpp
// loadFile(): decode WAV → m_decoded (interleaved float32, all in RAM)
// setEngine(): allocate m_inAB/m_outAB, Pa_OpenStream, Pa_StartStream
// play()/stop(): flip m_playing atomic
// doCallback() (audio thread, called every ~5ms):
//   - try_lock(m_chainMutex), output silence if unavailable
//   - de-interleave m_decoded[readPos..] into m_inAB
//   - engine.processBlock(inAB, outAB, frames)
//   - re-interleave outAB → PortAudio output buffer
//   - compute peak → m_outputLevel.store()
//   - advance m_readPos
```

Pre-allocated `AudioBuffer m_inAB, m_outAB` — no heap allocation in callback.
`DR_WAV_IMPLEMENTATION` defined in `GuiAudioIO.cpp` (no conflict with FileAudioIO.cpp since gearboxfx-gui does not link GearBoxFileSim).

---

## Third-Party Dependencies (new in Phase 1b)

| Library | Version | License | Use |
|---------|---------|---------|-----|
| GLFW | 3.4 | zlib/libpng | Window + OpenGL context |
| Dear ImGui | v1.91.6 | MIT | Immediate-mode GUI |
| imgui-knobs | main | MIT | Rotary knob widgets |

All fetched automatically via CMake `FetchContent`. No manual installation required.

---

## Smoke Test Checklist

1. Launch `gearboxfx-gui.exe` — window opens with 4 panels
2. Type a WAV path, click `[Load]` — no crash, file info in log
3. Click `[Play]` — audio comes from speakers, progress bar moves
4. Load `presets/04_chorus_clean.json` — chain panel shows 2 nodes
5. Click `chorus_1` in chain — param panel shows knobs (rate, depth, mix) + slider (voices doesn't appear as slider since range=3 > 2 — actually voices range is 1–4 = 3 → slider)
6. Move a knob/slider while playing — audio changes in real-time
7. `[+ Add Effect]` → `gain.overdrive` → new node appears in chain
8. `[Save As]` with name `my_test` → `presets/my_test.json` created
9. Close window — no crash

---

## Known Limitations (Phase 1b)

- WAV files > ~2 GB not supported (decoded fully into RAM)
- No drag-and-drop for WAV files (type path manually)
- No MIDI input yet (Phase 3)
- No BLE / hardware connection (Phase 2)
- Window layout resets if `gearboxfx_gui.ini` is deleted
- Looper and Tuner processors not wired to GUI yet
