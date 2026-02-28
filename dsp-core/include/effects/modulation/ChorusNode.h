#pragma once
#include "../../EffectNode.h"
#include <vector>

namespace gearboxfx {

// Multi-voice chorus using modulated delay lines (LFO-driven).
// Params: rate [0.1,8] Hz, depth [0,1], mix [0,1], voices [1,4]
class ChorusNode : public EffectNode {
public:
    ChorusNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    static constexpr int kMaxVoices    = 4;
    static constexpr int kMaxDelaySamp = 4096;  // ~85ms @ 48kHz

    float m_rate   = 0.5f;
    float m_depth  = 0.5f;
    float m_mix    = 0.5f;
    int   m_voices = 2;

    // Circular delay buffer (shared across voices, per channel)
    std::vector<float> m_delayBuf[2];  // channel 0, 1
    int                m_writePos = 0;

    // LFO phases for each voice
    float m_lfoPhase[kMaxVoices] = {};
    float m_lfoIncrement = 0.0f;

    float readDelayLinear(const std::vector<float>& buf, float delaySamps) const;
};

} // namespace gearboxfx
