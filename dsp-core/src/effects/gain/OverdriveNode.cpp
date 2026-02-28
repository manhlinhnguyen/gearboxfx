#include "effects/gain/OverdriveNode.h"
#include <cmath>

namespace gearboxfx {

static constexpr float kPi = 3.14159265358979f;

OverdriveNode::OverdriveNode() {
    registerParam("gain",  {0.5f, 0.0f, 1.0f, "Gain",  ""});
    registerParam("tone",  {0.5f, 0.0f, 1.0f, "Tone",  ""});
    registerParam("level", {0.7f, 0.0f, 1.0f, "Level", ""});
}

void OverdriveNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    m_toneState[0] = m_toneState[1] = 0.0f;
    recalcToneFilter();
}

void OverdriveNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcToneFilter();
    m_gain  = getParam("gain");
    m_tone  = getParam("tone");
    m_level = getParam("level");
}

void OverdriveNode::recalcToneFilter() {
    m_tone = getParam("tone");
    // Map tone [0,1] → cutoff [500, 8000] Hz
    float fc = 500.0f + m_tone * 7500.0f;
    double sr = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    // 1-pole IIR lowpass: y[n] = (1-a)*x[n] + a*y[n-1], where a = e^(-2π*fc/sr)
    m_toneCoeff = static_cast<float>(std::exp(-2.0 * kPi * fc / sr));
}

void OverdriveNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_gain  = getParam("gain");
    m_level = getParam("level");

    float driveAmount = 1.0f + m_gain * 20.0f;
    float invPiHalf   = 2.0f / kPi;
    float a           = m_toneCoeff;
    float oneMinusA   = 1.0f - a;

    for (int c = 0; c < output.numChannels; ++c) {
        int ch = (c < 2) ? c : 1;
        float& state = m_toneState[ch];

        for (int s = 0; s < numSamples; ++s) {
            float pre  = input[c][s] * driveAmount;
            float clip = invPiHalf * std::atan(pre);   // soft arctan saturation
            // 1-pole IIR tone filter
            state = oneMinusA * clip + a * state;
            output[c][s] = state * m_level;
        }
    }
}

} // namespace gearboxfx
