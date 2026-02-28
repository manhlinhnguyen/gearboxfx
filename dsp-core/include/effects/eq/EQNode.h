#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// 3-band parametric EQ: low-shelf (bass), peaking (mid), high-shelf (treble).
// All bands implemented as biquad filters (Audio EQ Cookbook formulas).
// Params: bass_db [-12,12], mid_db [-12,12], treble_db [-12,12], mid_freq [200,5000] Hz
class EQNode : public EffectNode {
public:
    EQNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    struct BiquadCoeffs {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;  // normalized (divided by a0)
    };

    struct BiquadState {
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;
    };

    // Band indices: 0=bass, 1=mid, 2=treble
    BiquadCoeffs m_coeffs[3];
    BiquadState  m_state[3][2];  // [band][channel]

    void recalcCoeffs();

    static BiquadCoeffs makeLowShelf (float gainDb, float freqHz, float sr);
    static BiquadCoeffs makePeaking  (float gainDb, float freqHz, float sr, float Q);
    static BiquadCoeffs makeHighShelf(float gainDb, float freqHz, float sr);

    static float processBiquad(float x, BiquadState& s, const BiquadCoeffs& c);
};

} // namespace gearboxfx
