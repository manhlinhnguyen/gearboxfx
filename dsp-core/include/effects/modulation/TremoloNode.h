#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// Amplitude modulation tremolo.
// Params: rate [0.1,20] Hz, depth [0,1], waveform [0=sine,1=triangle,2=square]
class TremoloNode : public EffectNode {
public:
    TremoloNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_rate      = 5.0f;
    float m_depth     = 0.7f;
    int   m_waveform  = 0;  // 0=sine, 1=triangle, 2=square

    float m_phase     = 0.0f;
    float m_increment = 0.0f;

    float computeLfo(float phase) const;
    void  recalcIncrement();
};

} // namespace gearboxfx
