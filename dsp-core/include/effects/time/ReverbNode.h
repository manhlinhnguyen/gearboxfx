#pragma once
#include "../../EffectNode.h"
#include <vector>
#include <array>

namespace gearboxfx {

// Freeverb-style reverb: 4 damped comb filters + 2 allpass filters.
// Params: size [0,1], decay [0,1], damping [0,1], pre_delay_ms [0,100], mix [0,1]
class ReverbNode : public EffectNode {
public:
    ReverbNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    static constexpr int kNumCombs    = 4;
    static constexpr int kNumAllpass  = 2;

    float m_size       = 0.5f;
    float m_decay      = 0.5f;
    float m_damping    = 0.5f;
    float m_preDelayMs = 10.0f;
    float m_mix        = 0.3f;

    // Base comb delays in ms (will be scaled by size)
    static constexpr float kCombBaseMs[kNumCombs]    = {29.7f, 37.1f, 41.1f, 43.7f};
    static constexpr float kAllpassBaseMs[kNumAllpass] = {5.0f, 1.7f};

    struct CombFilter {
        std::vector<float> buf;
        int   pos      = 0;
        float feedback = 0.5f;
        float damp     = 0.5f;
        float store    = 0.0f;
        void resize(int n) { buf.assign(n, 0.0f); }
        float process(float x);
    };

    struct AllpassFilter {
        std::vector<float> buf;
        int   pos      = 0;
        float feedback = 0.5f;
        void resize(int n) { buf.assign(n, 0.0f); }
        float process(float x);
    };

    // Separate L/R comb & allpass filter banks
    CombFilter   m_combs[2][kNumCombs];
    AllpassFilter m_allpass[2][kNumAllpass];

    // Pre-delay buffer
    std::vector<float> m_preDelayBuf[2];
    int                m_preDelayPos = 0;
    int                m_preDelaySamples = 0;

    void rebuildFilters();
};

} // namespace gearboxfx
