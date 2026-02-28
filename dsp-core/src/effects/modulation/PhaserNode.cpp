#include "effects/modulation/PhaserNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kPi    = 3.14159265358979f;
static constexpr float kTwoPi = 6.28318530717959f;

PhaserNode::PhaserNode() {
    registerParam("rate",     {0.5f, 0.1f, 5.0f, "Rate",     "Hz"});
    registerParam("depth",    {0.8f, 0.0f, 1.0f, "Depth",    ""});
    registerParam("feedback", {0.5f, 0.0f, 0.9f, "Feedback", ""});
    registerParam("mix",      {0.5f, 0.0f, 1.0f, "Mix",      ""});
}

void PhaserNode::onPrepare(double sampleRate, int /*maxBlockSize*/) {
    for (int st = 0; st < kNumStages; ++st)
        for (int ch = 0; ch < 2; ++ch) {
            m_apState[st][ch] = 0.0f;
            m_xPrev[st][ch]   = 0.0f;
        }
    m_feedbackState[0] = m_feedbackState[1] = 0.0f;
    m_lfoPhase     = 0.0f;
    m_lfoIncrement = getParam("rate") / static_cast<float>(sampleRate);
}

void PhaserNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    double sr      = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_lfoIncrement = getParam("rate") / static_cast<float>(sr);
}

void PhaserNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    float depth    = getParam("depth");
    float feedback = getParam("feedback");
    float mix      = getParam("mix");

    double sr     = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float freqMin = 100.0f;
    float freqMax = 3000.0f;
    float dryGain = 1.0f - mix;
    float wetGain = mix;

    for (int s = 0; s < numSamples; ++s) {
        float lfo = 0.5f * (1.0f + std::sin(kTwoPi * m_lfoPhase));  // [0, 1]
        float fc  = freqMin + (freqMax - freqMin) * lfo * depth;
        // First-order all-pass coefficient: k = (tan(π*fc/sr) - 1) / (tan(π*fc/sr) + 1)
        float tanFc = std::tan(kPi * fc / static_cast<float>(sr));
        float k     = (tanFc - 1.0f) / (tanFc + 1.0f);

        m_lfoPhase += m_lfoIncrement;
        if (m_lfoPhase >= 1.0f) m_lfoPhase -= 1.0f;

        for (int c = 0; c < output.numChannels; ++c) {
            int ch = (c < 2) ? c : 1;

            // Input with feedback from last all-pass output
            float x = input[c][s] + m_feedbackState[ch] * feedback;

            // Chain kNumStages first-order all-pass filters
            // Direct Form I: y[n] = k*x[n] + x[n-1] - k*y[n-1]
            for (int st = 0; st < kNumStages; ++st) {
                float y = k * x + m_xPrev[st][ch] - k * m_apState[st][ch];
                m_xPrev[st][ch]   = x;
                m_apState[st][ch] = y;
                x = y;
            }

            m_feedbackState[ch] = x;
            output[c][s] = input[c][s] * dryGain + x * wetGain;
        }
    }
}

} // namespace gearboxfx
