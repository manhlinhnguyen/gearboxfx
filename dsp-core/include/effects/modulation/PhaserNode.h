#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// 4-stage all-pass phaser with LFO-swept center frequency and feedback.
// Creates notch-filtering sweeps (classic guitar phaser sound).
// Params: rate [0.1,5] Hz, depth [0,1], feedback [0,0.9], mix [0,1]
class PhaserNode : public EffectNode {
public:
    PhaserNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    static constexpr int kNumStages = 4;

    // First-order all-pass state per stage per channel: y[n] = k*(x[n] - y[n-1]) + x[n-1]
    float m_apState[kNumStages][2] = {};  // [stage][channel]
    float m_feedbackState[2]       = {};  // last output per channel (for feedback loop)

    float m_lfoPhase     = 0.0f;
    float m_lfoIncrement = 0.0f;

    // x[n-1] state for each stage/channel (Direct Form I all-pass)
    float m_xPrev[kNumStages][2] = {};
};

} // namespace gearboxfx
