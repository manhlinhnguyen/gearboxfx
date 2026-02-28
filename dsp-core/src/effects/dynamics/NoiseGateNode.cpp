#include "effects/dynamics/NoiseGateNode.h"
#include <cmath>

namespace gearboxfx {

NoiseGateNode::NoiseGateNode() {
    registerParam("threshold_db", {-60.0f, -96.0f, 0.0f,   "Threshold", "dB"});
    registerParam("attack_ms",    {  5.0f,   0.1f, 100.0f, "Attack",    "ms"});
    registerParam("release_ms",   {100.0f,  10.0f, 2000.0f,"Release",   "ms"});
}

void NoiseGateNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    m_envelope = 0.0f;
    m_gate     = 0.0f;
    recalcCoeffs();
}

void NoiseGateNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcCoeffs();
}

void NoiseGateNode::recalcCoeffs() {
    float threshDb   = getParam("threshold_db");
    float attackMs   = getParam("attack_ms");
    float releaseMs  = getParam("release_ms");

    m_threshold = std::pow(10.0f, threshDb / 20.0f);

    double sr = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_attackCoeff  = static_cast<float>(std::exp(-1.0 / (sr * attackMs  * 0.001)));
    m_releaseCoeff = static_cast<float>(std::exp(-1.0 / (sr * releaseMs * 0.001)));
}

void NoiseGateNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    for (int s = 0; s < numSamples; ++s) {
        // Compute peak envelope from all channels
        float peak = 0.0f;
        for (int c = 0; c < input.numChannels; ++c)
            peak = std::max(peak, std::abs(input[c][s]));

        // Envelope follower: attack on rise, release on fall
        if (peak > m_envelope)
            m_envelope = peak + m_attackCoeff  * (m_envelope - peak);
        else
            m_envelope = peak + m_releaseCoeff * (m_envelope - peak);

        // Smooth gate state
        float target = (m_envelope > m_threshold) ? 1.0f : 0.0f;
        m_gate += 0.01f * (target - m_gate);

        for (int c = 0; c < output.numChannels; ++c)
            output[c][s] = input[c][s] * m_gate;
    }
}

} // namespace gearboxfx
