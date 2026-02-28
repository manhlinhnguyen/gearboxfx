// DR_WAV_IMPLEMENTATION must be defined exactly once per binary.
// GearBoxDesktopGui does not link GearBoxFileSim, so no conflict.
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include "GuiAudioIO.h"
#include <portaudio.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gearboxfx {

GuiAudioIO::GuiAudioIO() {
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        spdlog::error("GuiAudioIO: Pa_Initialize failed: {}", Pa_GetErrorText(err));
    } else {
        m_paInited = true;
    }
}

GuiAudioIO::~GuiAudioIO() {
    closeStream();
    if (m_paInited) {
        Pa_Terminate();
        m_paInited = false;
    }
}

bool GuiAudioIO::loadFile(const std::string& path) {
    m_playing.store(false);

    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) {
        spdlog::error("GuiAudioIO: cannot open '{}'", path);
        return false;
    }

    m_sr          = wav.sampleRate;
    m_numCh       = static_cast<int>(wav.channels);
    m_totalFrames = static_cast<uint64_t>(wav.totalPCMFrameCount);

    m_decoded.resize(m_totalFrames * static_cast<size_t>(m_numCh));
    drwav_read_pcm_frames_f32(&wav, m_totalFrames, m_decoded.data());
    drwav_uninit(&wav);

    m_readPos.store(0);
    m_outputLevel.store(0.0f);

    spdlog::info("GuiAudioIO: loaded '{}' — {}Hz, {}ch, {} frames",
                 path, m_sr, m_numCh, m_totalFrames);
    return true;
}

void GuiAudioIO::setEngine(EffectEngine* engine, double sampleRate, int blockSize) {
    m_playing.store(false);
    closeStream();

    m_engine     = engine;
    m_sampleRate = sampleRate;
    m_blockSize  = blockSize;

    int nCh = (m_numCh > 0) ? m_numCh : 2;
    m_inAB.resize(nCh, blockSize);
    m_outAB.resize(nCh, blockSize);

    if (!m_paInited || !engine || m_numCh == 0) return;

    double sr = (m_sr > 0) ? static_cast<double>(m_sr) : sampleRate;

    PaDeviceIndex dev = Pa_GetDefaultOutputDevice();
    if (dev == paNoDevice) {
        spdlog::warn("GuiAudioIO: no default output device");
        return;
    }

    PaStreamParameters outParams{};
    outParams.device                    = dev;
    outParams.channelCount              = m_numCh;
    outParams.sampleFormat              = paFloat32;
    outParams.suggestedLatency          = Pa_GetDeviceInfo(dev)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    PaError err = Pa_OpenStream(&m_stream, nullptr, &outParams,
                                sr, static_cast<unsigned long>(blockSize),
                                paClipOff, paCallback, this);
    if (err != paNoError) {
        spdlog::error("GuiAudioIO: Pa_OpenStream failed: {}", Pa_GetErrorText(err));
        m_stream = nullptr;
        return;
    }

    Pa_StartStream(m_stream);
    spdlog::info("GuiAudioIO: PortAudio stream open at {}Hz, {}ch, block={}",
                 (int)sr, m_numCh, blockSize);
}

void GuiAudioIO::play() {
    if (m_readPos.load() >= m_totalFrames)
        m_readPos.store(0);
    m_playing.store(true);
}

void GuiAudioIO::stop() {
    m_playing.store(false);
}

void GuiAudioIO::seekToStart() {
    m_readPos.store(0);
}

float GuiAudioIO::progress() const {
    if (m_totalFrames == 0) return 0.0f;
    return static_cast<float>(m_readPos.load()) /
           static_cast<float>(m_totalFrames);
}

void GuiAudioIO::closeStream() {
    if (m_stream) {
        Pa_StopStream(m_stream);
        Pa_CloseStream(m_stream);
        m_stream = nullptr;
    }
}

int GuiAudioIO::paCallback(const void* /*in*/, void* out,
                            unsigned long frames,
                            const PaStreamCallbackTimeInfo* /*ti*/,
                            PaStreamCallbackFlags /*flags*/,
                            void* userData)
{
    return static_cast<GuiAudioIO*>(userData)->doCallback(out, frames);
}

int GuiAudioIO::doCallback(void* outBuf, unsigned long frames) {
    auto* out   = static_cast<float*>(outBuf);
    int   numCh = m_numCh;
    int   nF    = static_cast<int>(frames);

    if (!m_playing.load() || m_engine == nullptr || m_decoded.empty()) {
        std::memset(out, 0, frames * static_cast<size_t>(numCh) * sizeof(float));
        return paContinue;
    }

    // Non-blocking chain lock: skip block (output silence) if GUI is modifying chain
    std::unique_lock<std::mutex> lock(m_chainMutex, std::try_to_lock);
    if (!lock.owns_lock()) {
        std::memset(out, 0, frames * static_cast<size_t>(numCh) * sizeof(float));
        return paContinue;
    }

    uint64_t readPos    = m_readPos.load();
    uint64_t framesLeft = (readPos < m_totalFrames) ? (m_totalFrames - readPos) : 0;

    if (framesLeft == 0) {
        if (m_loop.load()) {
            m_readPos.store(0);
            readPos = 0;
            framesLeft = m_totalFrames;
        } else {
            m_playing.store(false);
            std::memset(out, 0, frames * static_cast<size_t>(numCh) * sizeof(float));
            return paContinue;
        }
    }

    auto toRead = static_cast<int>(std::min(static_cast<uint64_t>(nF), framesLeft));

    // De-interleave from decoded buffer into planar inAB
    const float* src = m_decoded.data() + readPos * static_cast<size_t>(numCh);
    for (int f = 0; f < toRead; ++f)
        for (int c = 0; c < numCh; ++c)
            m_inAB.getWritePointer(c)[f] = src[f * numCh + c];

    // Zero-pad tail samples (end of file within this block)
    for (int f = toRead; f < nF; ++f)
        for (int c = 0; c < numCh; ++c)
            m_inAB.getWritePointer(c)[f] = 0.0f;

    AudioBufferView inView  = m_inAB.view();
    AudioBufferView outView = m_outAB.view();
    inView.numChannels  = outView.numChannels  = numCh;
    inView.numSamples   = outView.numSamples   = nF;

    m_engine->processBlock(inView, outView, nF);

    // Re-interleave and compute peak level
    float peak = 0.0f;
    for (int f = 0; f < nF; ++f)
        for (int c = 0; c < numCh; ++c) {
            float v = m_outAB.getReadPointer(c)[f];
            out[f * numCh + c] = v;
            float a = (v < 0.0f) ? -v : v;
            if (a > peak) peak = a;
        }

    m_outputLevel.store(peak);
    m_readPos.store(readPos + static_cast<uint64_t>(toRead));
    return paContinue;
}

} // namespace gearboxfx
