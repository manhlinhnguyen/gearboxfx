#include "effects/modulation/TremoloNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kTwoPi = 6.28318530717959f;

TremoloNode::TremoloNode() {
    registerParam("rate",     { 5.0f,  0.1f, 20.0f, "Rate",     "Hz"});
    registerParam("depth",    { 0.7f,  0.0f,  1.0f, "Depth",    ""});
    registerParam("waveform", { 0.0f,  0.0f,  2.0f, "Waveform", ""});  // 0=sine,1=tri,2=square
}

void TremoloNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    m_phase = 0.0f;
    recalcIncrement();
}

void TremoloNode::onParamChanged(const std::string& name, float /*value*/) {
    if (name == "rate")      recalcIncrement();
    if (name == "depth")     m_depth    = getParam("depth");
    if (name == "waveform")  m_waveform = static_cast<int>(getParam("waveform") + 0.5f);
}

void TremoloNode::recalcIncrement() {
    double sr   = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_rate      = getParam("rate");
    m_increment = m_rate / static_cast<float>(sr);
}

float TremoloNode::computeLfo(float phase) const {
    switch (m_waveform) {
        default:
        case 0:  // Sine
            return std::sin(kTwoPi * phase);
        case 1:  // Triangle
            return (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
        case 2:  // Square
            return (phase < 0.5f) ? 1.0f : -1.0f;
    }
}

void TremoloNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_depth    = getParam("depth");
    m_waveform = static_cast<int>(getParam("waveform") + 0.5f);

    for (int s = 0; s < numSamples; ++s) {
        float lfo = computeLfo(m_phase);
        // Map lfo [-1,1] → gain [1-depth, 1]
        float gain = 1.0f - m_depth * (1.0f - lfo) * 0.5f;

        for (int c = 0; c < output.numChannels; ++c)
            output[c][s] = input[c][s] * gain;

        m_phase += m_increment;
        if (m_phase >= 1.0f) m_phase -= 1.0f;
    }
}

} // namespace gearboxfx
