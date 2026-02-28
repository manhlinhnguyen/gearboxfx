#include "effects/output/VolumeNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

VolumeNode::VolumeNode() {
    registerParam("volume_db",           {0.0f, -60.0f, 12.0f, "Volume",    "dB"});
    registerParam("limiter_threshold_db", {0.0f, -18.0f,  0.0f, "Limiter",  "dB"});
}

void VolumeNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcGains();
}

void VolumeNode::recalcGains() {
    float volDb   = getParam("volume_db");
    float limDb   = getParam("limiter_threshold_db");
    m_linearGain  = std::pow(10.0f, volDb  / 20.0f);
    m_limThresh   = std::pow(10.0f, limDb  / 20.0f);
}

void VolumeNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    float gain   = m_linearGain;
    float thresh = m_limThresh;

    // Limiter time constants (~1ms attack, ~100ms release @ 48kHz)
    double sr        = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float  attackC   = std::exp(-1.0 / (0.001 * sr));
    float  releaseC  = std::exp(-1.0 / (0.100 * sr));

    for (int s = 0; s < numSamples; ++s) {
        for (int c = 0; c < output.numChannels; ++c) {
            int ch = (c < 2) ? c : 1;
            float x = input[c][s] * gain;

            // Peak envelope follower
            float absX = std::abs(x);
            float coeff = (absX > m_envState[ch]) ? attackC : releaseC;
            m_envState[ch] = absX + coeff * (m_envState[ch] - absX);

            // Soft-knee gain reduction above threshold
            if (m_envState[ch] > thresh && thresh > 0.0f) {
                float reduction = thresh / m_envState[ch];
                x *= reduction;
            }

            output[c][s] = x;
        }
    }
}

} // namespace gearboxfx
