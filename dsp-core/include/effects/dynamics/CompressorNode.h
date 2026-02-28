#pragma once
#include "../../EffectNode.h"

namespace gearboxfx {

// RMS compressor with soft knee.
// Params: threshold_db, ratio, attack_ms, release_ms, makeup_db, knee_db
class CompressorNode : public EffectNode {
public:
    CompressorNode();
    void process(AudioBufferView input, AudioBufferView output, int numSamples) override;

protected:
    void onPrepare(double sampleRate, int maxBlockSize) override;
    void onParamChanged(const std::string& name, float value) override;

private:
    float m_thresholdLin = 1.0f;
    float m_ratio        = 4.0f;
    float m_makeupLin    = 1.0f;
    float m_kneeDb       = 6.0f;
    float m_attackCoeff  = 0.999f;
    float m_releaseCoeff = 0.9999f;
    float m_envelope     = 0.0f;  // dB envelope

    float computeGainDb(float inputDb) const;
    void  recalcCoeffs();
};

} // namespace gearboxfx
