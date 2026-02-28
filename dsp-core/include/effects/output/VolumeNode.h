#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Output volume control with soft-knee brick-wall limiter.
// Params: volume_db [-60,12], limiter_threshold_db [-18,0]
class VolumeNode : public EffectNode {
public:
    VolumeNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_linearGain = 1.0f;
    float m_limThresh  = 1.0f;   // linear threshold

    // Envelope follower for limiter gain reduction (per-channel)
    float m_envState[2] = {};

    void recalcGains();
};

} // namespace gearboxfx
