#pragma once
#include "../../EffectNode.h"
#include <vector>

namespace gearboxfx {

// Mono/stereo digital delay with Hermite interpolation.
// Params: time_ms [1,2000], feedback [0,0.99], mix [0,1], bpm_sync [0,1], bpm [60,240]
class DelayNode : public EffectNode {
public:
    DelayNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    static constexpr int kMaxDelaySamples = 96001;  // 2000ms @ 48kHz

    float m_timeMs   = 300.0f;
    float m_feedback = 0.4f;
    float m_mix      = 0.5f;
    bool  m_bpmSync  = false;
    float m_bpm      = 120.0f;

    float m_delaySamples = 14400.0f;  // fractional

    std::vector<float> m_buf[2];
    int                m_writePos = 0;

    float hermiteRead(const std::vector<float>& buf, float delaySamps) const;
    void  recalcDelaySamples();
};

} // namespace gearboxfx
