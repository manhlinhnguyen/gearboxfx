#pragma once
#include "IAudioIO.h"
#include "EffectEngine.h"
#include <string>
#include <functional>

namespace gearboxfx {

enum class FileSimMode {
    FileToFile,    // WAV input → process → WAV output
    FileToSpeaker  // WAV input → process → PortAudio speaker
};

struct FileSimConfig {
    std::string   inputPath;
    std::string   outputPath;   // used only in FileToFile mode
    FileSimMode   mode         = FileSimMode::FileToFile;
    uint32_t      bufferSize   = 256;
    int           deviceIndex  = -1;  // -1 = default PortAudio device
};

// File-based IAudioIO implementation for Phase 1a laptop simulation.
// FileToFile: decode WAV → process → encode WAV (offline, no PortAudio).
// FileToSpeaker: decode WAV → process → stream to speakers via PortAudio.
class FileAudioIO : public IAudioIO {
public:
    explicit FileAudioIO(EffectEngine& engine, FileSimConfig cfg);
    ~FileAudioIO() override;

    bool open(const AudioFormat& fmt) override;
    void start() override;
    void stop() override;
    void close() override;

    const AudioFormat& format() const override { return m_fmt; }

    // Callback for progress reporting: receives [0.0, 1.0]
    void setProgressCallback(std::function<void(float)> cb) { m_progressCb = cb; }

private:
    bool runFileToFile();
    bool runFileToSpeaker();

    EffectEngine&  m_engine;
    FileSimConfig  m_cfg;
    AudioFormat    m_fmt;
    bool           m_running = false;

    std::function<void(float)> m_progressCb;
};

} // namespace gearboxfx
