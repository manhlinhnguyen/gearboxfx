#include "effects/gain/DistortionNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kPi = 3.14159265358979f;

DistortionNode::DistortionNode() {
    registerParam("gain",       {0.7f, 0.0f, 1.0f, "Gain",       ""});
    registerParam("tone",       {0.5f, 0.0f, 1.0f, "Tone",       ""});
    registerParam("level",      {0.8f, 0.0f, 1.0f, "Level",      ""});
    registerParam("asymmetry",  {0.3f, 0.0f, 1.0f, "Asymmetry",  ""});
}

void DistortionNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    m_hpState[0] = m_hpState[1] = 0.0f;
    m_hpPrev [0] = m_hpPrev [1] = 0.0f;
    m_lpState[0] = m_lpState[1] = 0.0f;
    recalcThresholds();
    recalcLpFilter();
}

void DistortionNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcThresholds();
    recalcLpFilter();
    m_gain      = getParam("gain");
    m_tone      = getParam("tone");
    m_level     = getParam("level");
    m_asymmetry = getParam("asymmetry");
}

void DistortionNode::recalcThresholds() {
    float g     = getParam("gain");
    m_posThresh = 0.9f - g * 0.3f;   // [0.9, 0.6]
    m_negThresh = 1.0f - g * 0.2f;   // [1.0, 0.8]
}

void DistortionNode::recalcLpFilter() {
    float tone = getParam("tone");
    // Map tone [0,1] → fc [800, 6000]
    float fc   = 800.0f + tone * 5200.0f;
    double sr  = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_lpCoeff  = static_cast<float>(std::exp(-2.0 * kPi * fc / sr));
}

void DistortionNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_gain      = getParam("gain");
    m_level     = getParam("level");
    m_asymmetry = getParam("asymmetry");

    // HP coefficient: RC highpass ~20Hz
    double sr = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float hpCoeff = static_cast<float>(std::exp(-2.0 * kPi * 20.0 / sr));

    float lpA       = m_lpCoeff;
    float lpOneMinA = 1.0f - lpA;
    float posT      = m_posThresh;
    float negT      = m_negThresh + m_asymmetry * 0.2f;

    // Pre-gain: drive harder into clipper
    float driveGain = 1.0f + m_gain * 30.0f;

    for (int c = 0; c < output.numChannels; ++c) {
        int ch = (c < 2) ? c : 1;

        for (int s = 0; s < numSamples; ++s) {
            // High-pass at input (DC blocker)
            float x   = input[c][s];
            float hp  = x - m_hpPrev[ch] + hpCoeff * m_hpState[ch];
            m_hpPrev[ch]  = x;
            m_hpState[ch] = hp;

            // Drive into hard clipper
            float driven = hp * driveGain;
            float clipped;
            if (driven > posT)
                clipped = posT;
            else if (driven < -negT)
                clipped = -negT;
            else
                clipped = driven;

            // 1-pole LP tone filter
            m_lpState[ch] = lpOneMinA * clipped + lpA * m_lpState[ch];

            output[c][s] = m_lpState[ch] * m_level;
        }
    }
}

} // namespace gearboxfx
