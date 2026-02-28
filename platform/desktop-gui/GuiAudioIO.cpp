// dr_libs: define implementation macros once per binary (no conflict — GUI exe
// does not link GearBoxFileSim).
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// stb_vorbis: compiled as a separate C translation unit (stb_vorbis_impl.c)
// to avoid the #define C macro polluting C++ template code in spdlog/fmtlib.
extern "C" {
    int stb_vorbis_decode_filename(const char* filename,
                                   int* channels, int* sample_rate,
                                   short** output);
}

#include "GuiAudioIO.h"
#include <portaudio.h>
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace {

// Returns the lowercase file extension including the dot, e.g. ".mp3"
static std::string fileExtLower(const std::string& path) {
    auto pos = path.rfind('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

// Decode WAV / MP3 / OGG into an interleaved float32 buffer.
// Sets sr, numCh, totalFrames on success and returns true.
static bool decodeAudioFile(const std::string& path,
                             std::vector<float>& out,
                             uint32_t& sr, int& numCh, uint64_t& totalFrames)
{
    const std::string ext = fileExtLower(path);

    if (ext == ".mp3") {
        drmp3 mp3;
        if (!drmp3_init_file(&mp3, path.c_str(), nullptr)) return false;
        sr      = mp3.sampleRate;
        numCh   = static_cast<int>(mp3.channels);
        drmp3_uint64 fc = drmp3_get_pcm_frame_count(&mp3);
        totalFrames = static_cast<uint64_t>(fc);
        out.resize(totalFrames * static_cast<size_t>(numCh));
        drmp3_read_pcm_frames_f32(&mp3, fc, out.data());
        drmp3_uninit(&mp3);
        return true;
    }

    if (ext == ".ogg") {
        int ch = 0, rate = 0;
        short* rawData = nullptr;
        int nFrames = stb_vorbis_decode_filename(path.c_str(), &ch, &rate, &rawData);
        if (nFrames < 0 || !rawData) return false;
        sr          = static_cast<uint32_t>(rate);
        numCh       = ch;
        totalFrames = static_cast<uint64_t>(nFrames);
        out.resize(totalFrames * static_cast<size_t>(numCh));
        for (size_t i = 0; i < out.size(); ++i)
            out[i] = rawData[i] / 32768.0f;
        std::free(rawData);
        return true;
    }

    // Default: WAV (also handles unknown extensions)
    drwav wav;
    if (!drwav_init_file(&wav, path.c_str(), nullptr)) return false;
    sr          = wav.sampleRate;
    numCh       = static_cast<int>(wav.channels);
    totalFrames = static_cast<uint64_t>(wav.totalPCMFrameCount);
    out.resize(totalFrames * static_cast<size_t>(numCh));
    drwav_read_pcm_frames_f32(&wav, totalFrames, out.data());
    drwav_uninit(&wav);
    return true;
}

} // anonymous namespace

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

    // Decode into a temporary buffer so the chain lock is held for the
    // shortest possible time (disk I/O must not happen under the lock).
    std::vector<float> decoded;
    uint32_t sr = 0; int numCh = 0; uint64_t totalFrames = 0;
    if (!decodeAudioFile(path, decoded, sr, numCh, totalFrames)) {
        spdlog::error("GuiAudioIO: cannot open '{}'", path);
        return false;
    }

    // Swap decoded data under the chain mutex so the audio callback never
    // reads m_decoded / m_numCh / m_totalFrames while they are being replaced.
    {
        std::lock_guard<std::mutex> lock(m_chainMutex);
        m_decoded = std::move(decoded);
        m_sr      = sr;
        m_numCh.store(numCh);
        m_totalFrames.store(totalFrames);
    }

    m_readPos.store(0);
    m_outputLevel.store(0.0f);

    spdlog::info("GuiAudioIO: loaded '{}' — {}Hz, {}ch, {} frames",
                 path, m_sr, m_numCh.load(), m_totalFrames.load());
    return true;
}

void GuiAudioIO::setEngine(EffectEngine* engine, double sampleRate, int blockSize) {
    m_playing.store(false);
    closeStream();

    m_engine     = engine;
    m_sampleRate = sampleRate;
    m_blockSize  = blockSize;

    int nCh = (m_numCh.load() > 0) ? m_numCh.load() : 2;
    m_inAB.resize(nCh, blockSize);
    m_outAB.resize(nCh, blockSize);

    if (!m_paInited || !engine || m_numCh.load() == 0) return;

    double sr = (m_sr > 0) ? static_cast<double>(m_sr) : sampleRate;

    PaDeviceIndex dev = Pa_GetDefaultOutputDevice();
    if (dev == paNoDevice) {
        spdlog::warn("GuiAudioIO: no default output device");
        return;
    }

    PaStreamParameters outParams{};
    outParams.device                    = dev;
    outParams.channelCount              = m_numCh.load();
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
                 (int)sr, m_numCh.load(), blockSize);
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
    if (m_totalFrames.load() == 0) return 0.0f;
    return static_cast<float>(m_readPos.load()) /
           static_cast<float>(m_totalFrames.load());
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

    if (!m_playing.load() || m_engine == nullptr || m_totalFrames.load() == 0) {
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
