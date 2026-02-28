#include "effects/modulation/ChorusNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kPi     = 3.14159265358979f;
static constexpr float kTwoPi  = 6.28318530717959f;

ChorusNode::ChorusNode() {
    registerParam("rate",   {0.5f, 0.1f, 8.0f, "Rate",   "Hz"});
    registerParam("depth",  {0.5f, 0.0f, 1.0f, "Depth",  ""});
    registerParam("mix",    {0.5f, 0.0f, 1.0f, "Mix",    ""});
    registerParam("voices", {2.0f, 1.0f, 4.0f, "Voices", ""});
}

void ChorusNode::onPrepare(double sampleRate, int /*maxBlockSize*/) {
    for (int c = 0; c < 2; ++c) {
        m_delayBuf[c].assign(kMaxDelaySamp, 0.0f);
    }
    m_writePos = 0;

    float rate = getParam("rate");
    m_lfoIncrement = rate / static_cast<float>(sampleRate);

    int voices = static_cast<int>(getParam("voices"));
    for (int v = 0; v < kMaxVoices; ++v)
        m_lfoPhase[v] = static_cast<float>(v) / static_cast<float>(std::max(1, voices));
}

void ChorusNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    double sr      = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float rate     = getParam("rate");
    m_lfoIncrement = rate / static_cast<float>(sr);
    m_voices       = std::max(1, std::min(kMaxVoices, (int)getParam("voices")));
}

float ChorusNode::readDelayLinear(const std::vector<float>& buf, float delaySamps) const {
    int   d0 = static_cast<int>(delaySamps);
    float fr = delaySamps - static_cast<float>(d0);

    int idx0 = (m_writePos - d0 + kMaxDelaySamp) % kMaxDelaySamp;
    int idx1 = (m_writePos - d0 - 1 + kMaxDelaySamp) % kMaxDelaySamp;

    return buf[idx0] * (1.0f - fr) + buf[idx1] * fr;
}

void ChorusNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_mix    = getParam("mix");
    m_depth  = getParam("depth");
    m_voices = std::max(1, std::min(kMaxVoices, (int)getParam("voices")));

    // Base delay center: ~20ms, depth modulates ±10ms
    double sr      = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float  baseDel = static_cast<float>(0.020 * sr);
    float  modAmp  = static_cast<float>(0.010 * sr) * m_depth;

    float dryGain = 1.0f - m_mix;
    float wetGain = m_mix / static_cast<float>(m_voices);

    for (int s = 0; s < numSamples; ++s) {
        for (int c = 0; c < output.numChannels; ++c)
            output[c][s] = input[c][s] * dryGain;

        // Write current input into delay buffer
        for (int c = 0; c < std::min(2, input.numChannels); ++c)
            m_delayBuf[c][m_writePos] = input[c][s];

        // Accumulate voices
        for (int v = 0; v < m_voices; ++v) {
            float lfo = std::sin(kTwoPi * m_lfoPhase[v]);
            float del = baseDel + modAmp * lfo;
            del = std::max(1.0f, std::min(del, (float)(kMaxDelaySamp - 2)));

            for (int c = 0; c < output.numChannels; ++c) {
                int ch = std::min(c, 1);
                output[c][s] += readDelayLinear(m_delayBuf[ch], del) * wetGain;
            }

            m_lfoPhase[v] += m_lfoIncrement;
            if (m_lfoPhase[v] >= 1.0f) m_lfoPhase[v] -= 1.0f;
        }

        m_writePos = (m_writePos + 1) % kMaxDelaySamp;
    }
}

} // namespace gearboxfx
