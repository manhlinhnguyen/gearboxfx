#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Noise gate with envelope follower (peak decay).
// Params: threshold_db, attack_ms, release_ms
class NoiseGateNode : public EffectNode {
public:
    NoiseGateNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_threshold    = 0.01f;  // linear amplitude
    float m_attackCoeff  = 0.99f;
    float m_releaseCoeff = 0.9999f;
    float m_envelope     = 0.0f;
    float m_gate         = 0.0f;   // smooth gate state [0,1]

    void recalcCoeffs();
};

} // namespace gearboxfx
