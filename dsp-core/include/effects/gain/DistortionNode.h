#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Hard-clip distortion with asymmetric threshold + HP input + LP tone output.
// Params: gain [0,1], tone [0,1], level [0,1], asymmetry [0,1]
class DistortionNode : public EffectNode {
public:
    DistortionNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_gain       = 0.7f;
    float m_tone       = 0.5f;
    float m_level      = 0.8f;
    float m_asymmetry  = 0.3f;

    float m_posThresh  = 0.6f;
    float m_negThresh  = 0.8f;

    // HP filter state (DC blocker at input, per channel)
    float m_hpState[2] = {0.0f, 0.0f};
    float m_hpPrev [2] = {0.0f, 0.0f};

    // LP tone filter state (per channel)
    float m_lpState[2] = {0.0f, 0.0f};
    float m_lpCoeff    = 0.5f;

    void recalcThresholds();
    void recalcLpFilter();
};

} // namespace gearboxfx
