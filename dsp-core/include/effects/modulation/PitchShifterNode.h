#pragma once
#include "../../EffectNode.h"
#include <vector>

namespace gearboxfx {

// Granular pitch shifter using two overlapping read heads with Hann-window crossfade.
// Params: semitones [-12,12], mix [0,1]
class PitchShifterNode : public EffectNode {
public:
    PitchShifterNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;

private:
    static constexpr int kGrainSize = 2048;  // ~42ms @ 48kHz
    static constexpr int kBufSize   = kGrainSize * 2;

    std::vector<float> m_buf[2];  // per-channel circular buffer
    int   m_writePos = 0;
    float m_readPos[2] = {};       // fractional read position per head (0..kGrainSize)

    // Hermite cubic interpolation
    float readHermite(const std::vector<float>& buf, float pos) const;
};

} // namespace gearboxfx
