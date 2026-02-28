#pragma once
#include <vector>
#include <cstring>
#include <cassert>

namespace gearboxfx {

// Non-owning view into a multi-channel interleaved float buffer.
// Samples are stored as: [L0, R0, L1, R1, ...] (interleaved)
// or as separate channel pointers (planar).
// This view stores planar pointers for DSP convenience.
struct AudioBufferView {
    float** channelData  = nullptr;
    int     numChannels  = 0;
    int     numSamples   = 0;

    float* operator[](int ch) { return channelData[ch]; }
    const float* operator[](int ch) const { return channelData[ch]; }

    void clear() {
        for (int c = 0; c < numChannels; ++c)
            std::memset(channelData[c], 0, numSamples * sizeof(float));
    }
};

// Owning multi-channel planar float buffer.
class AudioBuffer {
public:
    AudioBuffer() = default;

    AudioBuffer(int numChannels, int numSamples)
        : m_numChannels(numChannels), m_numSamples(numSamples)
    {
        resize(numChannels, numSamples);
    }

    void resize(int numChannels, int numSamples) {
        m_numChannels = numChannels;
        m_numSamples  = numSamples;
        m_data.assign(numChannels * numSamples, 0.0f);
        m_ptrs.resize(numChannels);
        for (int c = 0; c < numChannels; ++c)
            m_ptrs[c] = m_data.data() + c * numSamples;
    }

    void clear() {
        std::fill(m_data.begin(), m_data.end(), 0.0f);
    }

    // Copy samples from another buffer (same layout required).
    void copyFrom(const AudioBuffer& src) {
        assert(src.m_numChannels == m_numChannels && src.m_numSamples == m_numSamples);
        m_data = src.m_data;
    }

    float* getWritePointer(int ch) { return m_ptrs[ch]; }
    const float* getReadPointer(int ch) const { return m_ptrs[ch]; }

    int numChannels() const { return m_numChannels; }
    int numSamples()  const { return m_numSamples; }

    AudioBufferView view() {
        return { m_ptrs.data(), m_numChannels, m_numSamples };
    }

private:
    int                  m_numChannels = 0;
    int                  m_numSamples  = 0;
    std::vector<float>   m_data;
    std::vector<float*>  m_ptrs;
};

} // namespace gearboxfx
