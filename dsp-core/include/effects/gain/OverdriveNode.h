#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Tube-style overdrive: arctan soft clipper + IIR tone filter.
// Params: gain [0,1], tone [0,1], level [0,1]
class OverdriveNode : public EffectNode {
public:
    OverdriveNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_gain  = 0.5f;
    float m_tone  = 0.5f;
    float m_level = 0.7f;

    // IIR 1-pole low-pass tone filter state (per channel, max 2)
    float m_toneState[2] = {0.0f, 0.0f};
    float m_toneCoeff    = 0.5f;  // a1 coefficient

    void recalcToneFilter();
};

} // namespace gearboxfx
