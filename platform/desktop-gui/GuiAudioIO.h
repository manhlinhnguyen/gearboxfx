#pragma once
#include "EffectEngine.h"
#include "AudioBuffer.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// Forward-declare PortAudio types so consumers of this header
// do not need portaudio's include directory.
typedef void PaStream;
typedef unsigned long PaStreamCallbackFlags;
struct PaStreamCallbackTimeInfo;

namespace gearboxfx {

// Real-time PortAudio playback engine for the desktop GUI.
// Loads a WAV file into memory, then streams it through EffectEngine
// via a PortAudio callback. All playback state is accessed via atomics.
class GuiAudioIO {
public:
    GuiAudioIO();
    ~GuiAudioIO();

    // Decode a WAV file into memory. Stops current playback.
    bool loadFile(const std::string& path);

    // Bind engine and open/reopen the PortAudio output stream.
    // Must be called after loadFile() so numChannels/sampleRate are known.
    void setEngine(EffectEngine* engine, double sampleRate, int blockSize);

    void play();
    void stop();
    void seekToStart();

    bool     isPlaying()   const { return m_playing.load(); }
    float    progress()    const;           // [0,1]
    float    outputLevel() const { return m_outputLevel.load(); }
    int      numChannels() const { return m_numCh; }
    uint32_t sampleRate()  const { return m_sr; }
    bool     hasFile()     const { return !m_decoded.empty(); }
    bool     loop()        const { return m_loop.load(); }
    void     setLoop(bool l)    { m_loop.store(l); }

    // Acquire chain mutex before any structural chain modification (add/remove/move).
    // The returned lock is released automatically on destruction.
    std::unique_lock<std::mutex> lockChain() {
        return std::unique_lock<std::mutex>(m_chainMutex);
    }

private:
    EffectEngine* m_engine     = nullptr;
    double        m_sampleRate = 48000.0;
    int           m_blockSize  = 256;

    std::vector<float> m_decoded;      // interleaved float32 frames
    int      m_numCh       = 0;
    uint32_t m_sr          = 0;
    uint64_t m_totalFrames = 0;

    std::atomic<uint64_t> m_readPos    {0};
    std::atomic<bool>     m_playing    {false};
    std::atomic<float>    m_outputLevel{0.0f};
    std::atomic<bool>     m_loop       {false};

    PaStream* m_stream   = nullptr;
    bool      m_paInited = false;

    // Pre-allocated DSP buffers (sized in setEngine, never reallocated in callback)
    AudioBuffer m_inAB;
    AudioBuffer m_outAB;

    // Guards processBlock against concurrent structural chain changes
    std::mutex m_chainMutex;

    void closeStream();

    static int paCallback(const void* in, void* out, unsigned long frames,
                          const PaStreamCallbackTimeInfo* ti,
                          PaStreamCallbackFlags flags, void* userData);
    int doCallback(void* outBuf, unsigned long frames);
};

} // namespace gearboxfx
