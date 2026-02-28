#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Clean boost: simple linear gain applied to all samples.
// Params: gain_db  [-20, +20]
class CleanBoostNode : public EffectNode {
public:
    CleanBoostNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_gainLin = 1.0f;
};

} // namespace gearboxfx
