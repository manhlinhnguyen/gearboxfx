Để tôi phân tích kỹ và thiết kế toàn bộ code trước.Để tôi thiết kế toàn bộ kiến trúc Phase 1a trước.

**Phase 1a bao gồm những gì?**

Thay vì dùng guitar + soundcard thật, ta xây dựng một **File Audio Pipeline** thay thế hoàn toàn lớp HAL — file WAV/MP3 đóng vai trò nguồn tín hiệu đầu vào, output là file WAV đã xử lý HOẶC phát thẳng ra loa laptop. DSP Core không thay đổi một dòng nào.

```
[WAV/MP3 File] → Decode → [DSP Core] → [Output: Loa / WAV file]
     ↑                        ↑
  Thay cho ADC            Giống hệt
                          hardware thật
```

---

```
gearbox-fx/
│
├── CMakeLists.txt                    ← Root CMake
├── CMakePresets.json                 ← Build presets (laptop/stm32/bela)
├── vcpkg.json                        ← Dependency manifest (vcpkg)
│
├── dsp-core/                         ← MODULE DÙNG CHUNG (không đổi khi chuyển platform)
│   ├── CMakeLists.txt
│   ├── include/
│   │   ├── IAudioIO.h                ← HAL interface
│   │   ├── AudioBuffer.h             ← Buffer helper
│   │   ├── EffectNode.h              ← Base class mọi effect
│   │   ├── EffectEngine.h            ← Điều phối chính
│   │   ├── EffectChain.h             ← Signal graph (ping-pong)
│   │   ├── ParameterManager.h        ← Thread-safe param store
│   │   ├── PresetStore.h             ← Load/Save JSON
│   │   ├── TunerProcessor.h          ← YIN pitch detection
│   │   ├── LooperProcessor.h         ← Looper (Phase 1b)
│   │   └── effects/
│   │       ├── EffectNodeRegistry.h  ← Factory, tự đăng ký
│   │       ├── dynamics/
│   │       │   ├── NoiseGateNode.h
│   │       │   └── CompressorNode.h
│   │       ├── gain/
│   │       │   ├── OverdriveNode.h
│   │       │   ├── DistortionNode.h
│   │       │   └── CleanBoostNode.h
│   │       ├── modulation/
│   │       │   ├── ChorusNode.h
│   │       │   ├── FlangerNode.h
│   │       │   ├── PhaserNode.h
│   │       │   └── TremoloNode.h
│   │       └── time/
│   │           ├── DelayNode.h
│   │           └── ReverbNode.h
│   └── src/
│       ├── EffectEngine.cpp
│       ├── EffectChain.cpp
│       ├── PresetStore.cpp
│       ├── TunerProcessor.cpp
│       └── effects/
│           ├── dynamics/
│           │   ├── NoiseGateNode.cpp
│           │   └── CompressorNode.cpp
│           ├── gain/
│           │   ├── OverdriveNode.cpp
│           │   ├── DistortionNode.cpp
│           │   └── CleanBoostNode.cpp
│           ├── modulation/
│           │   ├── ChorusNode.cpp
│           │   ├── FlangerNode.cpp
│           │   ├── PhaserNode.cpp
│           │   └── TremoloNode.cpp
│           └── time/
│               ├── DelayNode.cpp
│               └── ReverbNode.cpp
│
├── platform/
│   ├── file-sim/                     ← Platform cho Phase 1a (WAV/MP3 pipeline)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── FileAudioIO.h         ← HAL impl: đọc file → DSP → ghi file
│   │   │   ├── AudioFileReader.h     ← Decode WAV/MP3
│   │   │   ├── AudioFileWriter.h     ← Encode WAV output
│   │   │   └── RealtimePlayer.h      ← Phát real-time qua loa (PortAudio)
│   │   └── src/
│   │       ├── FileAudioIO.cpp
│   │       ├── AudioFileReader.cpp
│   │       ├── AudioFileWriter.cpp
│   │       └── RealtimePlayer.cpp
│   │
│   ├── laptop/                       ← Phase 1b: JUCE GUI app (sau)
│   ├── stm32/                        ← Phase 2: STM32H7 (sau)
│   └── bela/                         ← Phase 2: Bela Board (sau)
│
├── app/                              ← CLI application Phase 1a
│   ├── CMakeLists.txt
│   ├── main.cpp                      ← Entry point: CLI arg parsing
│   ├── CLI.h/cpp                     ← Interactive CLI controller
│   └── PresetValidator.h/cpp         ← Validate JSON schema
│
├── presets/                          ← Factory presets JSON
│   ├── 01_clean_boost.json
│   ├── 02_mild_overdrive.json
│   ├── 03_heavy_distortion.json
│   ├── 04_chorus_clean.json
│   ├── 05_delay_reverb.json
│   └── schema.json                   ← JSON Schema for validation
│
├── test-audio/                       ← Sample audio files để test
│   ├── guitar_clean_riff.wav
│   ├── guitar_single_note.wav
│   └── guitar_chord_strum.wav
│
├── tests/                            ← Unit tests
│   ├── CMakeLists.txt
│   ├── test_effect_chain.cpp
│   ├── test_overdrive.cpp
│   ├── test_reverb.cpp
│   ├── test_delay.cpp
│   ├── test_preset_store.cpp
│   └── test_tuner.cpp
│
└── docs/
    ├── phase1a-setup.md
    └── dsp-developer-guide.md
```

```cmake
cmake_minimum_required(VERSION 3.24)
project(GearBoxFX VERSION 1.0.0 LANGUAGES CXX)

# ── C++ Standard ──────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Build type mặc định ───────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Platform selection ────────────────────────────────────────
# Dùng: cmake -DGBFX_PLATFORM=file-sim  (Phase 1a, mặc định)
#       cmake -DGBFX_PLATFORM=laptop    (Phase 1b, JUCE GUI)
#       cmake -DGBFX_PLATFORM=stm32     (Phase 2)
set(GBFX_PLATFORM "file-sim" CACHE STRING "Target platform")
set_property(CACHE GBFX_PLATFORM PROPERTY STRINGS
    file-sim laptop stm32 bela)

message(STATUS "╔══════════════════════════════════════╗")
message(STATUS "║   GearBox FX Build System            ║")
message(STATUS "║   Platform : ${GBFX_PLATFORM}        ║")
message(STATUS "║   BuildType: ${CMAKE_BUILD_TYPE}     ║")
message(STATUS "╚══════════════════════════════════════╝")

# ── Compiler warnings ─────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-parameter
        -ffast-math           # Tối ưu float math (safe cho audio DSP)
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O3)
    endif()
endif()

# ── Dependencies (vcpkg hoặc FetchContent) ────────────────────
include(FetchContent)

# nlohmann/json — JSON header-only library
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

# GoogleTest — Unit testing
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)

# dr_libs — WAV/MP3 decode (header-only, không cần build)
FetchContent_Declare(
    dr_libs
    GIT_REPOSITORY https://github.com/mackron/dr_libs.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

# PortAudio — Real-time audio output ra loa
FetchContent_Declare(
    portaudio
    GIT_REPOSITORY https://github.com/PortAudio/portaudio.git
    GIT_TAG        v19.7.0
    GIT_SHALLOW    TRUE
)

# spdlog — Logging
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.13.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(
    nlohmann_json
    googletest
    dr_libs
    portaudio
    spdlog
)

# ── Subdirectories ────────────────────────────────────────────
add_subdirectory(dsp-core)
add_subdirectory(platform/${GBFX_PLATFORM})
add_subdirectory(app)
add_subdirectory(tests)
```

```cpp
#pragma once
// ═══════════════════════════════════════════════════════════════
// IAudioIO.h — Hardware Abstraction Layer Interface
//
// Đây là "hợp đồng" duy nhất giữa DSP Core và platform.
// Mọi platform (file-sim, JUCE, STM32, Bela) chỉ cần implement
// interface này. DSP Core KHÔNG BIẾT mình đang chạy ở đâu.
// ═══════════════════════════════════════════════════════════════
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

// ─── Audio format descriptor ──────────────────────────────────
struct AudioFormat {
    float    sampleRate   = 48000.0f;
    uint32_t numChannels  = 1;         // 1 = mono (guitar), 2 = stereo
    uint32_t bufferSize   = 128;       // Số samples mỗi block
    uint32_t bitDepth     = 24;

    // Latency ước tính (ms) = bufferSize / sampleRate * 1000
    float estimatedLatencyMs() const {
        return (static_cast<float>(bufferSize) / sampleRate) * 1000.0f;
    }
};

// ─── Callback types ───────────────────────────────────────────
// Platform gọi callback này mỗi khi có block audio mới sẵn sàng
// input  : [numChannels × numSamples] interleaved float [-1.0, 1.0]
// output : [numChannels × numSamples] interleaved float [-1.0, 1.0]
using AudioProcessCallback = std::function<
    void(const float* input, float* output, std::size_t numSamples)
>;

// ─── IAudioIO interface ───────────────────────────────────────
class IAudioIO {
public:
    virtual ~IAudioIO() = default;

    // Khởi tạo với format và callback xử lý
    virtual bool initialize(const AudioFormat& format,
                            AudioProcessCallback callback) = 0;

    // Bắt đầu stream audio
    virtual bool start() = 0;

    // Dừng stream audio
    virtual void stop() = 0;

    // Giải phóng tài nguyên
    virtual void shutdown() = 0;

    // Lấy format hiện tại
    virtual AudioFormat getFormat() const = 0;

    // Trạng thái
    virtual bool isRunning() const = 0;

    // Tên platform (debug)
    virtual std::string platformName() const = 0;
};
```

```cpp
#pragma once
// ═══════════════════════════════════════════════════════════════
// AudioBuffer.h — Non-owning audio buffer view
//
// Thin wrapper quanh float* — zero overhead, không allocate memory.
// Dùng trong DSP Core để truyền buffer giữa các effect node
// mà không cần biết memory được cấp phát ở đâu.
// ═══════════════════════════════════════════════════════════════
#include <cstddef>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <vector>

// ─── AudioBufferView — Non-owning view (dùng trong process()) ─
class AudioBufferView {
public:
    AudioBufferView(float* data, std::size_t numSamples, std::size_t numChannels = 1)
        : m_data(data), m_numSamples(numSamples), m_numChannels(numChannels) {}

    float*       data()        { return m_data; }
    const float* data() const  { return m_data; }

    std::size_t numSamples()  const { return m_numSamples; }
    std::size_t numChannels() const { return m_numChannels; }

    // Truy cập sample tại channel ch, sample index s
    float& at(std::size_t ch, std::size_t s) {
        assert(ch < m_numChannels && s < m_numSamples);
        return m_data[ch * m_numSamples + s]; // non-interleaved
    }

    float at(std::size_t ch, std::size_t s) const {
        assert(ch < m_numChannels && s < m_numSamples);
        return m_data[ch * m_numSamples + s];
    }

    void clear() {
        std::fill(m_data, m_data + m_numSamples * m_numChannels, 0.0f);
    }

    void copyFrom(const AudioBufferView& other) {
        assert(m_numSamples == other.m_numSamples);
        std::copy(other.m_data,
                  other.m_data + m_numSamples * m_numChannels,
                  m_data);
    }

    // Clamp tất cả sample vào [-1.0, 1.0] tránh clipping DAC
    void clamp() {
        for (std::size_t i = 0; i < m_numSamples * m_numChannels; ++i)
            m_data[i] = std::max(-1.0f, std::min(1.0f, m_data[i]));
    }

private:
    float*      m_data;
    std::size_t m_numSamples;
    std::size_t m_numChannels;
};

// ─── AudioBuffer — Owning buffer (dùng để cấp phát) ──────────
class AudioBuffer {
public:
    explicit AudioBuffer(std::size_t numSamples = 0, std::size_t numChannels = 1)
        : m_numSamples(numSamples), m_numChannels(numChannels),
          m_data(numSamples * numChannels, 0.0f) {}

    void resize(std::size_t numSamples, std::size_t numChannels = 1) {
        m_numSamples  = numSamples;
        m_numChannels = numChannels;
        m_data.assign(numSamples * numChannels, 0.0f);
    }

    AudioBufferView view() {
        return AudioBufferView(m_data.data(), m_numSamples, m_numChannels);
    }

    float*       data()        { return m_data.data(); }
    const float* data() const  { return m_data.data(); }
    std::size_t  numSamples()  const { return m_numSamples; }
    std::size_t  numChannels() const { return m_numChannels; }
    std::size_t  totalSize()   const { return m_data.size(); }

    void clear() { std::fill(m_data.begin(), m_data.end(), 0.0f); }

private:
    std::size_t        m_numSamples;
    std::size_t        m_numChannels;
    std::vector<float> m_data;
};
```

```cpp
#pragma once
// ═══════════════════════════════════════════════════════════════
// EffectNode.h — Abstract base class cho mọi effect
//
// Để thêm effect mới:
//   1. Tạo class kế thừa EffectNode
//   2. Override process(), optionally onPrepare(), onParamChanged()
//   3. Đăng ký trong EffectNodeRegistry
// Không cần chạm vào bất kỳ file nào khác.
// ═══════════════════════════════════════════════════════════════
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// ─── Parameter value với range validation ─────────────────────
struct ParamDef {
    float defaultValue;
    float minValue;
    float maxValue;
    std::string label;      // Tên hiển thị ("Gain")
    std::string unit;       // Đơn vị ("dB", "Hz", "ms", "")
};

// ─── EffectNode ───────────────────────────────────────────────
class EffectNode {
public:
    // Metadata
    std::string id;       // Unique ID trong chain, ví dụ "od_1"
    std::string type;     // Registry key, ví dụ "gain.overdrive"
    bool        enabled = true;

    virtual ~EffectNode() = default;

    // ── Lifecycle ─────────────────────────────────────────────

    // Gọi khi format thay đổi — VÍ DỤ: recalculate filter coefficients
    // KHÔNG gọi trong audio thread
    void prepare(float sampleRate, std::size_t bufferSize) {
        m_sampleRate = sampleRate;
        m_bufferSize = bufferSize;
        resizePingPong(bufferSize);
        onPrepare(sampleRate, bufferSize);
    }

    // ── Audio processing ──────────────────────────────────────

    // Gọi trong audio thread mỗi block.
    // QUAN TRỌNG: KHÔNG allocate memory, KHÔNG lock mutex ở đây!
    void processEffect(const float* input, float* output, std::size_t numSamples) {
        if (!enabled) {
            // Bypass: copy thẳng input → output
            std::copy(input, input + numSamples, output);
            return;
        }
        process(input, output, numSamples);
    }

    // ── Parameters ────────────────────────────────────────────

    // Lấy giá trị tham số (có fallback về default nếu không tồn tại)
    float getParam(const std::string& name) const {
        auto it = m_params.find(name);
        if (it == m_params.end()) {
            auto defIt = m_paramDefs.find(name);
            return (defIt != m_paramDefs.end()) ? defIt->second.defaultValue : 0.0f;
        }
        return it->second;
    }

    // Set tham số — thread-safe nếu dùng atomic float (xem ParameterManager)
    // Tự động clamp vào [min, max] nếu có ParamDef
    void setParam(const std::string& name, float value) {
        auto defIt = m_paramDefs.find(name);
        if (defIt != m_paramDefs.end()) {
            value = std::max(defIt->second.minValue,
                             std::min(defIt->second.maxValue, value));
        }
        m_params[name] = value;
        onParamChanged(name, value);
    }

    // Lấy tất cả param definitions (cho UI)
    const std::unordered_map<std::string, ParamDef>& getParamDefs() const {
        return m_paramDefs;
    }

    // ── Serialization ─────────────────────────────────────────

    virtual json toJson() const {
        json j;
        j["id"]      = id;
        j["type"]    = type;
        j["enabled"] = enabled;
        json params;
        for (auto& [key, val] : m_params)
            params[key] = val;
        j["params"] = params;
        return j;
    }

    virtual void fromJson(const json& j) {
        id      = j.value("id",      "");
        type    = j.value("type",    "");
        enabled = j.value("enabled", true);
        if (j.contains("params") && j["params"].is_object()) {
            for (auto& [key, val] : j["params"].items()) {
                if (val.is_number())
                    setParam(key, val.get<float>());
            }
        }
    }

    // ── Display name (override trong subclass) ─────────────────
    virtual std::string displayName() const { return type; }
    virtual std::string category()    const { return "unknown"; }

protected:
    float       m_sampleRate = 48000.0f;
    std::size_t m_bufferSize = 128;

    // Param storage
    std::unordered_map<std::string, float>    m_params;
    std::unordered_map<std::string, ParamDef> m_paramDefs;

    // Helper để đăng ký param trong constructor của subclass
    void registerParam(const std::string& name, const ParamDef& def) {
        m_paramDefs[name] = def;
        m_params[name]    = def.defaultValue;
    }

    // ── Virtual hooks (override trong subclass) ────────────────

    // Gọi khi prepare() — tính toán coefficients, resize buffers
    virtual void onPrepare(float /*sampleRate*/, std::size_t /*bufferSize*/) {}

    // Gọi khi setParam() — update coefficients khi cần
    virtual void onParamChanged(const std::string& /*name*/, float /*value*/) {}

    // Hàm xử lý audio thực sự — PHẢI override
    virtual void process(const float* input, float* output, std::size_t numSamples) = 0;

private:
    // Internal ping-pong buffers (nếu subclass cần internal processing)
    std::vector<float> m_internal;

    void resizePingPong(std::size_t size) {
        if (m_internal.size() < size)
            m_internal.resize(size, 0.0f);
    }
};

using EffectNodePtr = std::shared_ptr<EffectNode>;
```

```cpp
// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/dynamics/NoiseGateNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>

class NoiseGateNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "dynamics.noise_gate";
    std::string displayName() const override { return "Noise Gate"; }
    std::string category()    const override { return "Dynamics"; }

    NoiseGateNode() {
        type = TYPE_ID;
        registerParam("threshold_db", {-60.0f, -96.0f, 0.0f,   "Threshold", "dB"});
        registerParam("release_ms",   {100.0f,  10.0f, 2000.0f, "Release",   "ms"});
        registerParam("attack_ms",    {5.0f,    0.1f,  50.0f,   "Attack",    "ms"});
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        updateCoeffs();
    }

    void onParamChanged(const std::string&, float) override { updateCoeffs(); }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float threshold = std::pow(10.0f, getParam("threshold_db") / 20.0f);

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Envelope follower (peak detector)
            float absIn = std::abs(input[i]);
            float coeff = (absIn > m_envelope) ? m_attackCoeff : m_releaseCoeff;
            m_envelope  = coeff * m_envelope + (1.0f - coeff) * absIn;

            // Gate: open nếu envelope > threshold, đóng nếu < threshold
            float targetGain = (m_envelope > threshold) ? 1.0f : 0.0f;
            // Smooth gain transition để tránh click
            m_gateGain += 0.01f * (targetGain - m_gateGain);

            output[i] = input[i] * m_gateGain;
        }
    }

private:
    float m_envelope    = 0.0f;
    float m_gateGain    = 0.0f;
    float m_attackCoeff = 0.99f;
    float m_releaseCoeff = 0.999f;

    void updateCoeffs() {
        auto timeToCoeff = [&](float ms) {
            return std::exp(-1.0f / (m_sampleRate * ms / 1000.0f));
        };
        m_attackCoeff  = timeToCoeff(getParam("attack_ms"));
        m_releaseCoeff = timeToCoeff(getParam("release_ms"));
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/dynamics/CompressorNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>

class CompressorNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "dynamics.compressor";
    std::string displayName() const override { return "Compressor"; }
    std::string category()    const override { return "Dynamics"; }

    CompressorNode() {
        type = TYPE_ID;
        registerParam("threshold_db", {-20.0f, -60.0f,  0.0f,   "Threshold", "dB"});
        registerParam("ratio",        {4.0f,    1.0f,   20.0f,   "Ratio",     ":1"});
        registerParam("attack_ms",    {10.0f,   0.1f,  200.0f,  "Attack",    "ms"});
        registerParam("release_ms",   {100.0f,  10.0f, 2000.0f, "Release",   "ms"});
        registerParam("makeup_db",    {0.0f,    0.0f,   24.0f,   "Makeup",    "dB"});
        registerParam("knee_db",      {6.0f,    0.0f,   24.0f,   "Knee",      "dB"});
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        updateCoeffs();
    }
    void onParamChanged(const std::string&, float) override { updateCoeffs(); }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float threshold = getParam("threshold_db");
        const float ratio     = getParam("ratio");
        const float knee      = getParam("knee_db");
        const float makeup    = std::pow(10.0f, getParam("makeup_db") / 20.0f);

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Level detection (RMS approximation với envelope follower)
            float level    = std::abs(input[i]);
            float levelDb  = (level > 1e-6f)
                             ? 20.0f * std::log10(level)
                             : -120.0f;

            // Envelope follower trên domain dB
            float coeff = (levelDb > m_envelopeDb) ? m_attackCoeff : m_releaseCoeff;
            m_envelopeDb = coeff * m_envelopeDb + (1.0f - coeff) * levelDb;

            // Gain computer với soft knee
            float gainDb = computeGain(m_envelopeDb, threshold, ratio, knee);

            // Smooth gain
            m_gainDb += 0.001f * (gainDb - m_gainDb);

            float gain = std::pow(10.0f, m_gainDb / 20.0f) * makeup;
            output[i]  = input[i] * gain;
        }
    }

private:
    float m_envelopeDb  = -120.0f;
    float m_gainDb      = 0.0f;
    float m_attackCoeff = 0.99f;
    float m_releaseCoeff = 0.999f;

    void updateCoeffs() {
        auto timeToCoeff = [&](float ms) {
            return std::exp(-1.0f / (m_sampleRate * ms / 1000.0f));
        };
        m_attackCoeff  = timeToCoeff(getParam("attack_ms"));
        m_releaseCoeff = timeToCoeff(getParam("release_ms"));
    }

    float computeGain(float inputDb, float threshold, float ratio, float knee) {
        float halfKnee = knee / 2.0f;
        float diff     = inputDb - threshold;

        if (diff < -halfKnee) return 0.0f; // Dưới knee: no compression

        if (diff > halfKnee) {
            // Trên knee: full compression
            return (1.0f / ratio - 1.0f) * diff;
        }

        // Trong soft knee: interpolate
        float t = (diff + halfKnee) / knee;
        return (1.0f / ratio - 1.0f) * t * t * halfKnee;
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/gain/OverdriveNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>
#include <array>

class OverdriveNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "gain.overdrive";
    std::string displayName() const override { return "Overdrive"; }
    std::string category()    const override { return "Gain"; }

    OverdriveNode() {
        type = TYPE_ID;
        registerParam("gain",  {0.5f, 0.0f, 1.0f, "Gain",  ""});
        registerParam("tone",  {0.5f, 0.0f, 1.0f, "Tone",  ""});
        registerParam("level", {0.7f, 0.0f, 1.0f, "Level", ""});
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        updateToneFilter();
    }
    void onParamChanged(const std::string& name, float) override {
        if (name == "tone") updateToneFilter();
    }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float gain  = getParam("gain");
        const float tone  = getParam("tone");
        const float level = getParam("level");
        const float drive = 1.0f + gain * 20.0f; // 1x – 21x pre-gain

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Pre-gain
            float x = input[i] * drive;

            // Oversampling 2x để giảm aliasing
            float s0 = softClip(x);
            float s1 = softClip(x * 0.5f + m_prevSample * 0.5f);
            float clipped = (s0 + s1) * 0.5f;
            m_prevSample = x;

            // IIR tone filter (low-pass blended với direct signal)
            m_filterState += m_filterCoeff * (clipped - m_filterState);
            float toned = m_filterState * (1.0f - tone) + clipped * tone;

            output[i] = toned * level;
        }
    }

private:
    float m_filterState = 0.0f;
    float m_filterCoeff = 0.5f;
    float m_prevSample  = 0.0f;

    // Arctan soft clipper — âm thanh ấm, "tubey"
    static float softClip(float x) {
        return (2.0f / M_PI) * std::atan(x);
    }

    void updateToneFilter() {
        // tone [0,1] → cutoff [500 Hz, 8000 Hz]
        float cutoffHz = 500.0f + getParam("tone") * 7500.0f;
        float omega    = 2.0f * static_cast<float>(M_PI) * cutoffHz / m_sampleRate;
        m_filterCoeff  = omega / (omega + 1.0f);
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/gain/DistortionNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>
#include <algorithm>

class DistortionNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "gain.distortion";
    std::string displayName() const override { return "Distortion"; }
    std::string category()    const override { return "Gain"; }

    DistortionNode() {
        type = TYPE_ID;
        registerParam("gain",      {0.6f, 0.0f, 1.0f, "Gain",      ""});
        registerParam("tone",      {0.5f, 0.0f, 1.0f, "Tone",      ""});
        registerParam("level",     {0.7f, 0.0f, 1.0f, "Level",     ""});
        registerParam("asymmetry", {0.3f, 0.0f, 1.0f, "Asymmetry", ""}); // 0=symmetric, 1=asymmetric
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        updateFilters();
    }
    void onParamChanged(const std::string& name, float) override {
        if (name == "tone") updateFilters();
    }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float gain      = getParam("gain");
        const float level     = getParam("level");
        const float asym      = getParam("asymmetry");
        const float drive     = 1.0f + gain * 50.0f; // Mạnh hơn overdrive

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Input HP filter (loại bỏ DC + thêm "bite")
            float hpOut = input[i] - m_hpState;
            m_hpState  += m_hpCoeff * hpOut;

            float x = hpOut * drive;

            // Asymmetric hard clipping (mô phỏng diode clipping)
            float clipPos = 0.6f + asym * 0.3f; // Positive clip threshold
            float clipNeg = -(1.0f - asym * 0.2f); // Negative clip threshold
            float clipped = std::max(clipNeg, std::min(clipPos, x));

            // Output LP filter (tone control)
            m_lpState += m_lpCoeff * (clipped - m_lpState);

            output[i] = m_lpState * level;
        }
    }

private:
    // High-pass filter state (input filter)
    float m_hpState = 0.0f;
    float m_hpCoeff = 0.995f;

    // Low-pass filter state (tone/output filter)
    float m_lpState = 0.0f;
    float m_lpCoeff = 0.5f;

    void updateFilters() {
        // HP: fixed cutoff ~80Hz (remove low-frequency mud)
        float hpCutoff = 80.0f;
        float hpOmega  = 2.0f * static_cast<float>(M_PI) * hpCutoff / m_sampleRate;
        m_hpCoeff      = 1.0f / (1.0f + hpOmega);

        // LP: tone [0,1] → cutoff [1000Hz, 12000Hz]
        float lpCutoff = 1000.0f + getParam("tone") * 11000.0f;
        float lpOmega  = 2.0f * static_cast<float>(M_PI) * lpCutoff / m_sampleRate;
        m_lpCoeff      = lpOmega / (lpOmega + 1.0f);
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/gain/CleanBoostNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>

class CleanBoostNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "gain.clean_boost";
    std::string displayName() const override { return "Clean Boost"; }
    std::string category()    const override { return "Gain"; }

    CleanBoostNode() {
        type = TYPE_ID;
        registerParam("gain_db", {6.0f, -6.0f, 24.0f, "Gain", "dB"});
    }

protected:
    void onParamChanged(const std::string&, float) override {
        m_gain = std::pow(10.0f, getParam("gain_db") / 20.0f);
    }

    void process(const float* input, float* output, std::size_t numSamples) override {
        for (std::size_t i = 0; i < numSamples; ++i)
            output[i] = std::max(-1.0f, std::min(1.0f, input[i] * m_gain));
    }

private:
    float m_gain = 2.0f; // default +6dB
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/modulation/ChorusNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>
#include <vector>

class ChorusNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "modulation.chorus";
    std::string displayName() const override { return "Chorus"; }
    std::string category()    const override { return "Modulation"; }

    ChorusNode() {
        type = TYPE_ID;
        registerParam("rate",   {0.5f, 0.1f, 5.0f,  "Rate",  "Hz"});
        registerParam("depth",  {0.5f, 0.0f, 1.0f,  "Depth", ""  });
        registerParam("mix",    {0.5f, 0.0f, 1.0f,  "Mix",   ""  });
        registerParam("voices", {2.0f, 1.0f, 4.0f,  "Voices","" });
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        // Delay line: tối đa 30ms chorus depth
        std::size_t maxDelay = static_cast<std::size_t>(0.03f * sampleRate) + 2;
        m_delayLine.assign(maxDelay, 0.0f);
        m_writePos = 0;
        // Reset LFO phases (mỗi voice lệch pha nhau)
        for (int v = 0; v < 4; ++v)
            m_lfoPhase[v] = static_cast<float>(v) * (static_cast<float>(M_PI) / 2.0f);
    }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float rate    = getParam("rate");
        const float depth   = getParam("depth");
        const float mix     = getParam("mix");
        const int   voices  = static_cast<int>(getParam("voices"));
        const float lfoInc  = 2.0f * static_cast<float>(M_PI) * rate / m_sampleRate;

        // Base delay: 5–15ms (center of chorus sweep)
        const float baseDelayMs    = 8.0f;
        const float depthDelayMs   = depth * 6.0f; // ±6ms sweep
        const std::size_t delayLen = m_delayLine.size();

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Write vào delay line
            m_delayLine[m_writePos] = input[i];

            float wet = 0.0f;
            for (int v = 0; v < voices; ++v) {
                // LFO sine wave cho mỗi voice
                float lfo = std::sin(m_lfoPhase[v]);
                m_lfoPhase[v] += lfoInc;
                if (m_lfoPhase[v] > 2.0f * M_PI) m_lfoPhase[v] -= 2.0f * M_PI;

                // Delay thay đổi theo LFO
                float delaySamples = (baseDelayMs + lfo * depthDelayMs)
                                     * m_sampleRate / 1000.0f;

                // Linear interpolation trong delay line
                wet += readDelayLinear(delaySamples, delayLen);
            }
            wet /= static_cast<float>(voices); // Normalize theo số voices

            output[i] = input[i] * (1.0f - mix) + wet * mix;
            m_writePos = (m_writePos + 1) % delayLen;
        }
    }

private:
    std::vector<float> m_delayLine;
    std::size_t        m_writePos = 0;
    float              m_lfoPhase[4] = {};

    float readDelayLinear(float delaySamples, std::size_t delayLen) {
        // Lấy sample từ delay line với linear interpolation
        std::size_t intDelay = static_cast<std::size_t>(delaySamples);
        float       frac     = delaySamples - static_cast<float>(intDelay);

        std::size_t pos0 = (m_writePos + delayLen - intDelay)     % delayLen;
        std::size_t pos1 = (m_writePos + delayLen - intDelay - 1) % delayLen;

        return m_delayLine[pos0] * (1.0f - frac) + m_delayLine[pos1] * frac;
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/modulation/TremoloNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <cmath>

class TremoloNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "modulation.tremolo";
    std::string displayName() const override { return "Tremolo"; }
    std::string category()    const override { return "Modulation"; }

    TremoloNode() {
        type = TYPE_ID;
        registerParam("rate",  {4.0f,  0.1f, 20.0f, "Rate",  "Hz"});
        registerParam("depth", {0.7f,  0.0f,  1.0f, "Depth", ""  });
        // waveform: 0=sine, 1=square, 2=triangle
        registerParam("waveform", {0.0f, 0.0f, 2.0f, "Wave", ""});
    }

protected:
    void process(const float* input, float* output, std::size_t numSamples) override {
        const float rate     = getParam("rate");
        const float depth    = getParam("depth");
        const int   waveform = static_cast<int>(getParam("waveform") + 0.5f);
        const float lfoInc   = 2.0f * static_cast<float>(M_PI) * rate / m_sampleRate;

        for (std::size_t i = 0; i < numSamples; ++i) {
            float lfo;
            switch (waveform) {
                case 1: // Square
                    lfo = (m_phase < static_cast<float>(M_PI)) ? 1.0f : -1.0f;
                    break;
                case 2: // Triangle
                    lfo = (m_phase < static_cast<float>(M_PI))
                          ? (2.0f * m_phase / static_cast<float>(M_PI) - 1.0f)
                          : (3.0f - 2.0f * m_phase / static_cast<float>(M_PI));
                    break;
                default: // Sine
                    lfo = std::sin(m_phase);
                    break;
            }
            m_phase += lfoInc;
            if (m_phase > 2.0f * M_PI) m_phase -= 2.0f * M_PI;

            // Amplitude modulation: gain = 1 - depth * (1 - lfo) / 2
            float gain = 1.0f - depth * (1.0f - lfo) * 0.5f;
            output[i]  = input[i] * gain;
        }
    }

private:
    float m_phase = 0.0f;
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/time/DelayNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <vector>
#include <cmath>

class DelayNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "time.delay";
    std::string displayName() const override { return "Delay"; }
    std::string category()    const override { return "Time"; }

    DelayNode() {
        type = TYPE_ID;
        registerParam("time_ms",  {300.0f, 1.0f, 2000.0f, "Time",     "ms"});
        registerParam("feedback", {0.4f,   0.0f, 0.95f,   "Feedback", ""  });
        registerParam("mix",      {0.4f,   0.0f, 1.0f,    "Mix",      ""  });
        registerParam("bpm_sync", {0.0f,   0.0f, 1.0f,    "BPM Sync", ""  });
        registerParam("bpm",      {120.0f, 40.0f,240.0f,  "BPM",      ""  });
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        // Max delay: 2 giây
        std::size_t maxSamples = static_cast<std::size_t>(2.0f * sampleRate) + 2;
        m_buffer.assign(maxSamples, 0.0f);
        m_writePos = 0;
    }

    void onParamChanged(const std::string&, float) override {}

    void process(const float* input, float* output, std::size_t numSamples) override {
        float timeMs;
        if (getParam("bpm_sync") > 0.5f) {
            // BPM sync: 1 quarter note = 60000/BPM ms
            timeMs = 60000.0f / std::max(1.0f, getParam("bpm"));
        } else {
            timeMs = getParam("time_ms");
        }

        const float feedback    = getParam("feedback");
        const float mix         = getParam("mix");
        const float delaySamples = timeMs * m_sampleRate / 1000.0f;
        const std::size_t bufLen = m_buffer.size();

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Đọc từ delay line với Hermite interpolation (chất lượng cao hơn linear)
            float delayed = readHermite(delaySamples, bufLen);

            // Ghi vào delay line (input + feedback)
            m_buffer[m_writePos] = input[i] + delayed * feedback;
            m_writePos = (m_writePos + 1) % bufLen;

            output[i] = input[i] * (1.0f - mix) + delayed * mix;
        }
    }

private:
    std::vector<float> m_buffer;
    std::size_t        m_writePos = 0;

    // Hermite 4-point interpolation — chất lượng cao hơn linear
    float readHermite(float delaySamples, std::size_t bufLen) {
        std::size_t intD = static_cast<std::size_t>(delaySamples);
        float       t    = delaySamples - static_cast<float>(intD);

        auto idx = [&](int offset) -> float {
            std::size_t pos = (m_writePos + bufLen - intD + offset) % bufLen;
            return m_buffer[pos];
        };

        float y0 = idx(1), y1 = idx(0), y2 = idx(-1), y3 = idx(-2);

        // Hermite coefficients
        float c0 = y1;
        float c1 = (y2 - y0) * 0.5f;
        float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        float c3 = (y3 - y0) * 0.5f + (y1 - y2) * 1.5f;

        return ((c3 * t + c2) * t + c1) * t + c0;
    }
};


// ════════════════════════════════════════════════════════════════
// FILE: dsp-core/include/effects/time/ReverbNode.h
// ════════════════════════════════════════════════════════════════
#pragma once
#include "../../EffectNode.h"
#include <vector>
#include <array>
#include <cmath>

// Comb filter với damping (mô phỏng hấp thụ âm của phòng)
class DampedCombFilter {
public:
    void init(std::size_t delayInSamples, float feedback, float damping) {
        m_buffer.assign(delayInSamples, 0.0f);
        m_feedback = feedback;
        m_damping  = damping;
        m_pos      = 0;
        m_filterz  = 0.0f;
    }

    float process(float input) {
        float output   = m_buffer[m_pos];
        m_filterz      = output * (1.0f - m_damping) + m_filterz * m_damping;
        m_buffer[m_pos] = input + m_filterz * m_feedback;
        m_pos = (m_pos + 1) % m_buffer.size();
        return output;
    }

private:
    std::vector<float> m_buffer;
    float  m_feedback = 0.5f;
    float  m_damping  = 0.3f;
    float  m_filterz  = 0.0f;
    std::size_t m_pos = 0;
};

// Allpass filter — tán xạ âm thanh
class AllpassFilter {
public:
    void init(std::size_t delayInSamples, float gain) {
        m_buffer.assign(delayInSamples, 0.0f);
        m_gain = gain;
        m_pos  = 0;
    }

    float process(float input) {
        float bufOut = m_buffer[m_pos];
        float v      = input - bufOut * m_gain;
        m_buffer[m_pos] = v;
        m_pos = (m_pos + 1) % m_buffer.size();
        return bufOut + v * m_gain;
    }

private:
    std::vector<float> m_buffer;
    float       m_gain = 0.5f;
    std::size_t m_pos  = 0;
};

class ReverbNode : public EffectNode {
public:
    static constexpr const char* TYPE_ID = "time.reverb";
    std::string displayName() const override { return "Reverb"; }
    std::string category()    const override { return "Time"; }

    ReverbNode() {
        type = TYPE_ID;
        registerParam("size",         {0.5f, 0.1f, 1.0f,  "Size",      ""  });
        registerParam("decay",        {0.6f, 0.1f, 0.99f, "Decay",     ""  });
        registerParam("damping",      {0.3f, 0.0f, 1.0f,  "Damping",   ""  });
        registerParam("pre_delay_ms", {20.0f,0.0f, 100.0f,"Pre-Delay", "ms"});
        registerParam("mix",          {0.3f, 0.0f, 1.0f,  "Mix",       ""  });
    }

protected:
    void onPrepare(float sampleRate, std::size_t) override {
        m_sampleRate = sampleRate;
        initFilters();
        initPreDelay();
    }
    void onParamChanged(const std::string& name, float) override {
        if (name == "size" || name == "decay" || name == "damping") initFilters();
        if (name == "pre_delay_ms") initPreDelay();
    }

    void process(const float* input, float* output, std::size_t numSamples) override {
        const float mix = getParam("mix");

        for (std::size_t i = 0; i < numSamples; ++i) {
            // Pre-delay
            float predelayed = processPreDelay(input[i]);

            // 4 comb filters in parallel
            float combOut = 0.0f;
            for (auto& comb : m_combs)
                combOut += comb.process(predelayed);
            combOut *= 0.25f;

            // 2 allpass filters in series (diffusion)
            float diffused = m_allpass1.process(combOut);
            diffused       = m_allpass2.process(diffused);

            output[i] = input[i] * (1.0f - mix) + diffused * mix;
        }
    }

private:
    static constexpr std::array<float, 4> COMB_DELAYS_MS = {29.7f, 37.1f, 41.1f, 43.7f};

    std::array<DampedCombFilter, 4> m_combs;
    AllpassFilter m_allpass1, m_allpass2;

    std::vector<float> m_preDelayBuf;
    std::size_t        m_preDelayPos     = 0;
    std::size_t        m_preDelaySamples = 0;

    void initFilters() {
        float size    = getParam("size");
        float decay   = getParam("decay");
        float damping = getParam("damping");

        for (std::size_t k = 0; k < 4; ++k) {
            auto samples = static_cast<std::size_t>(
                COMB_DELAYS_MS[k] * size * m_sampleRate / 1000.0f);
            m_combs[k].init(std::max(samples, (std::size_t)1), decay * 0.97f, damping);
        }

        m_allpass1.init(static_cast<std::size_t>(5.0f * m_sampleRate / 1000.0f), 0.7f);
        m_allpass2.init(static_cast<std::size_t>(1.7f * m_sampleRate / 1000.0f), 0.7f);
    }

    void initPreDelay() {
        float ms    = getParam("pre_delay_ms");
        m_preDelaySamples = static_cast<std::size_t>(ms * m_sampleRate / 1000.0f);
        m_preDelayBuf.assign(m_preDelaySamples + 1, 0.0f);
        m_preDelayPos = 0;
    }

    float processPreDelay(float in) {
        if (m_preDelaySamples == 0) return in;
        float out = m_
```