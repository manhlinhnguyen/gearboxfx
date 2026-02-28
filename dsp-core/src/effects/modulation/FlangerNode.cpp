#include "effects/modulation/FlangerNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kTwoPi = 6.28318530717959f;

FlangerNode::FlangerNode() {
    registerParam("rate",     {0.3f, 0.1f,  5.0f,  "Rate",     "Hz"});
    registerParam("depth",    {0.8f, 0.0f,  1.0f,  "Depth",    ""});
    registerParam("feedback", {0.5f, -0.95f, 0.95f, "Feedback", ""});
    registerParam("mix",      {0.5f, 0.0f,  1.0f,  "Mix",      ""});
}

void FlangerNode::onPrepare(double sampleRate, int /*maxBlockSize*/) {
    for (int c = 0; c < 2; ++c)
        m_delayBuf[c].assign(kMaxDelaySamp, 0.0f);
    m_writePos   = 0;
    m_lfoPhase   = 0.0f;
    float rate   = getParam("rate");
    m_lfoIncrement = rate / static_cast<float>(sampleRate);
}

void FlangerNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    double sr    = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_lfoIncrement = getParam("rate") / static_cast<float>(sr);
}

float FlangerNode::readDelayLinear(const std::vector<float>& buf, float delaySamps) const {
    int   d0  = static_cast<int>(delaySamps);
    float fr  = delaySamps - static_cast<float>(d0);
    int   i0  = (m_writePos - d0 + kMaxDelaySamp)     % kMaxDelaySamp;
    int   i1  = (m_writePos - d0 - 1 + kMaxDelaySamp) % kMaxDelaySamp;
    return buf[i0] * (1.0f - fr) + buf[i1] * fr;
}

void FlangerNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    float rate     = getParam("rate");
    float depth    = getParam("depth");
    float feedback = getParam("feedback");
    float mix      = getParam("mix");
    (void)rate;

    double sr   = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    // Flanger delay range: 1ms – 7ms, modulated around center 4ms
    float center = static_cast<float>(0.004 * sr);
    float modAmp = static_cast<float>(0.003 * sr) * depth;

    float dryGain = 1.0f - mix;
    float wetGain = mix;

    for (int s = 0; s < numSamples; ++s) {
        float lfo = std::sin(kTwoPi * m_lfoPhase);
        float del = center + modAmp * lfo;
        del = std::max(1.0f, std::min(del, static_cast<float>(kMaxDelaySamp - 2)));

        for (int c = 0; c < output.numChannels; ++c) {
            int ch  = (c < 2) ? c : 1;
            float wet = readDelayLinear(m_delayBuf[ch], del);
            output[c][s] = input[c][s] * dryGain + wet * wetGain;
            // Write input + feedback into delay buffer
            m_delayBuf[ch][m_writePos] = input[c][s] + wet * feedback;
        }

        m_lfoPhase += m_lfoIncrement;
        if (m_lfoPhase >= 1.0f) m_lfoPhase -= 1.0f;

        m_writePos = (m_writePos + 1) % kMaxDelaySamp;
    }
}

} // namespace gearboxfx
