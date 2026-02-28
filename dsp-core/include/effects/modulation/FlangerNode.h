#pragma once
#include "../../EffectNode.h"
#include <vector>

namespace gearboxfx {

// Flanger: short modulated delay line (1–15 ms) with feedback.
// Creates comb-filtering sweeps (jet-engine / swoosh sound).
// Params: rate [0.1,5] Hz, depth [0,1], feedback [-0.95,0.95], mix [0,1]
class FlangerNode : public EffectNode {
public:
    FlangerNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    static constexpr int kMaxDelaySamp = 2048;  // ~42ms @ 48kHz — plenty for flanger

    std::vector<float> m_delayBuf[2];
    int   m_writePos     = 0;
    float m_lfoPhase     = 0.0f;
    float m_lfoIncrement = 0.0f;

    float readDelayLinear(const std::vector<float>& buf, float delaySamps) const;
};

} // namespace gearboxfx
