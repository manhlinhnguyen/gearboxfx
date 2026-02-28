// dr_libs: define implementation macros once per binary (no conflict — CLI exe
// does not link GearBoxDesktopGui).
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"

// stb_vorbis: compiled as a separate C translation unit (stb_vorbis_impl.c).
extern "C" {
    int stb_vorbis_decode_filename(const char* filename,
                                   int* channels, int* sample_rate,
                                   short** output);
}

#include "FileAudioIO.h"
#include "AudioBuffer.h"
#include <portaudio.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace {

static std::string fileExtLower(const std::string& path) {
    auto pos = path.rfind('.');
    if (pos == std::string::npos) return "";
    std::string ext = path.substr(pos);
    for (auto& c : ext)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return ext;
}

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

    // Default: WAV
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

// ── PortAudio stream callback data ─────────────────────────────────────────
struct PaStreamData {
    EffectEngine*            engine   = nullptr;
    const float*             samples  = nullptr;  // interleaved decoded samples
    size_t                   totalFrames = 0;
    size_t                   readPos     = 0;
    int                      numChannels = 2;
    int                      bufferSize  = 256;
    bool                     done        = false;
};

static int paCallback(const void* inputBuffer, void* outputBuffer,
                      unsigned long framesPerBuffer,
                      const PaStreamCallbackTimeInfo* /*timeInfo*/,
                      PaStreamCallbackFlags /*statusFlags*/,
                      void* userData)
{
    auto* data = static_cast<PaStreamData*>(userData);
    auto* out  = static_cast<float*>(outputBuffer);
    (void)inputBuffer;

    size_t framesLeft = data->totalFrames - data->readPos;
    if (framesLeft == 0) {
        std::memset(out, 0, framesPerBuffer * data->numChannels * sizeof(float));
        data->done = true;
        return paComplete;
    }

    int numCh    = data->numChannels;
    size_t frames = std::min(static_cast<size_t>(framesPerBuffer), framesLeft);

    // De-interleave into planar buffers
    AudioBuffer inBuf (numCh, static_cast<int>(framesPerBuffer));
    AudioBuffer outBuf(numCh, static_cast<int>(framesPerBuffer));

    const float* src = data->samples + data->readPos * numCh;
    for (size_t f = 0; f < frames; ++f)
        for (int c = 0; c < numCh; ++c)
            inBuf.getWritePointer(c)[f] = src[f * numCh + c];

    // Zero-pad the tail
    for (size_t f = frames; f < framesPerBuffer; ++f)
        for (int c = 0; c < numCh; ++c)
            inBuf.getWritePointer(c)[f] = 0.0f;

    auto inView  = inBuf.view();
    auto outView = outBuf.view();
    inView.numChannels  = outView.numChannels  = numCh;
    inView.numSamples   = outView.numSamples   = static_cast<int>(framesPerBuffer);

    data->engine->processBlock(inView, outView, static_cast<int>(framesPerBuffer));

    // Re-interleave output
    for (size_t f = 0; f < framesPerBuffer; ++f)
        for (int c = 0; c < numCh; ++c)
            out[f * numCh + c] = outBuf.getReadPointer(c)[f];

    data->readPos += frames;
    if (data->readPos >= data->totalFrames) {
        data->done = true;
        return paComplete;
    }
    return paContinue;
}

// ── FileAudioIO ─────────────────────────────────────────────────────────────
FileAudioIO::FileAudioIO(EffectEngine& engine, FileSimConfig cfg)
    : m_engine(engine), m_cfg(std::move(cfg))
{}

FileAudioIO::~FileAudioIO() {
    close();
}

bool FileAudioIO::open(const AudioFormat& fmt) {
    m_fmt = fmt;
    return true;
}

void FileAudioIO::start() {
    m_running = true;
    bool ok = (m_cfg.mode == FileSimMode::FileToFile) ? runFileToFile() : runFileToSpeaker();
    if (!ok)
        spdlog::error("FileAudioIO: processing failed");
    m_running = false;
}

void FileAudioIO::stop() {
    m_running = false;
}

void FileAudioIO::close() {
    m_running = false;
}

bool FileAudioIO::runFileToFile() {
    // ── Decode input audio (WAV / MP3 / OGG) ────────────────────────────────
    std::vector<float> decoded;
    uint32_t sr = 0; int numCh = 0; uint64_t totalFrames64 = 0;
    if (!decodeAudioFile(m_cfg.inputPath, decoded, sr, numCh, totalFrames64)) {
        spdlog::error("FileAudioIO: cannot open '{}'", m_cfg.inputPath);
        return false;
    }
    size_t totalFrames = static_cast<size_t>(totalFrames64);

    spdlog::info("FileAudioIO: input '{}' — {}Hz, {}ch, {} frames",
                 m_cfg.inputPath, sr, numCh, totalFrames);

    m_fmt.sampleRate  = sr;
    m_fmt.numChannels = static_cast<uint32_t>(numCh);
    m_fmt.bufferSize  = m_cfg.bufferSize;

    m_engine.prepare(static_cast<double>(sr), static_cast<int>(m_cfg.bufferSize));

    // ── Process in blocks ───────────────────────────────────────────────────
    int  blockSize  = static_cast<int>(m_cfg.bufferSize);
    std::vector<float> encoded(totalFrames * numCh);

    AudioBuffer inBuf (numCh, blockSize);
    AudioBuffer outBuf(numCh, blockSize);

    size_t offset = 0;
    while (offset < totalFrames) {
        size_t block = std::min(static_cast<size_t>(blockSize), totalFrames - offset);

        // De-interleave
        const float* src = decoded.data() + offset * numCh;
        for (size_t f = 0; f < block; ++f)
            for (int c = 0; c < numCh; ++c)
                inBuf.getWritePointer(c)[f] = src[f * numCh + c];

        // Zero-pad tail
        for (size_t f = block; f < static_cast<size_t>(blockSize); ++f)
            for (int c = 0; c < numCh; ++c)
                inBuf.getWritePointer(c)[f] = 0.0f;

        auto inView  = inBuf.view();
        auto outView = outBuf.view();
        inView.numChannels  = outView.numChannels  = numCh;
        inView.numSamples   = outView.numSamples   = static_cast<int>(block);

        m_engine.processBlock(inView, outView, static_cast<int>(block));

        // Re-interleave
        float* dst = encoded.data() + offset * numCh;
        for (size_t f = 0; f < block; ++f)
            for (int c = 0; c < numCh; ++c)
                dst[f * numCh + c] = outBuf.getReadPointer(c)[f];

        offset += block;

        if (m_progressCb)
            m_progressCb(static_cast<float>(offset) / static_cast<float>(totalFrames));
    }

    // ── Write output WAV ────────────────────────────────────────────────────
    drwav_data_format fmt{};
    fmt.container     = drwav_container_riff;
    fmt.format        = DR_WAVE_FORMAT_IEEE_FLOAT;
    fmt.channels      = static_cast<drwav_uint32>(numCh);
    fmt.sampleRate    = sr;
    fmt.bitsPerSample = 32;

    drwav outWav;
    if (!drwav_init_file_write(&outWav, m_cfg.outputPath.c_str(), &fmt, nullptr)) {
        spdlog::error("FileAudioIO: cannot write '{}'", m_cfg.outputPath);
        return false;
    }

    drwav_write_pcm_frames(&outWav, totalFrames, encoded.data());
    drwav_uninit(&outWav);

    spdlog::info("FileAudioIO: wrote '{}' ({} frames)", m_cfg.outputPath, totalFrames);
    return true;
}

bool FileAudioIO::runFileToSpeaker() {
    // ── Decode input audio (WAV / MP3 / OGG) ────────────────────────────────
    std::vector<float> decoded;
    uint32_t sr = 0; int numCh = 0; uint64_t totalFrames64 = 0;
    if (!decodeAudioFile(m_cfg.inputPath, decoded, sr, numCh, totalFrames64)) {
        spdlog::error("FileAudioIO: cannot open '{}'", m_cfg.inputPath);
        return false;
    }
    size_t totalFrames = static_cast<size_t>(totalFrames64);

    m_fmt.sampleRate  = sr;
    m_fmt.numChannels = static_cast<uint32_t>(numCh);
    m_fmt.bufferSize  = m_cfg.bufferSize;

    m_engine.prepare(static_cast<double>(sr), static_cast<int>(m_cfg.bufferSize));

    // ── PortAudio setup ─────────────────────────────────────────────────────
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        spdlog::error("PortAudio init: {}", Pa_GetErrorText(err));
        return false;
    }

    PaStreamData streamData;
    streamData.engine      = &m_engine;
    streamData.samples     = decoded.data();
    streamData.totalFrames = totalFrames;
    streamData.readPos     = 0;
    streamData.numChannels = numCh;
    streamData.bufferSize  = static_cast<int>(m_cfg.bufferSize);

    PaStream* stream = nullptr;
    PaStreamParameters outParams{};
    outParams.device = (m_cfg.deviceIndex >= 0)
                         ? m_cfg.deviceIndex
                         : Pa_GetDefaultOutputDevice();
    outParams.channelCount              = numCh;
    outParams.sampleFormat              = paFloat32;
    outParams.suggestedLatency          =
        Pa_GetDeviceInfo(outParams.device)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(&stream, nullptr, &outParams,
                        static_cast<double>(sr),
                        m_cfg.bufferSize,
                        paClipOff,
                        paCallback,
                        &streamData);
    if (err != paNoError) {
        spdlog::error("PortAudio OpenStream: {}", Pa_GetErrorText(err));
        Pa_Terminate();
        return false;
    }

    spdlog::info("FileAudioIO: streaming '{}' to speakers...", m_cfg.inputPath);

    Pa_StartStream(stream);

    // Wait until done
    while (!streamData.done && m_running)
        Pa_Sleep(50);

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();

    spdlog::info("FileAudioIO: playback complete");
    return true;
}

} // namespace gearboxfx
