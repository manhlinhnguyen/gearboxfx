#include "effects/dynamics/CompressorNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

CompressorNode::CompressorNode() {
    registerParam("threshold_db", {-18.0f, -60.0f,  0.0f,  "Threshold", "dB"});
    registerParam("ratio",        {  4.0f,   1.0f, 20.0f,  "Ratio",     ":1"});
    registerParam("attack_ms",    { 10.0f,   0.1f, 200.0f, "Attack",    "ms"});
    registerParam("release_ms",   {100.0f,  10.0f, 2000.0f,"Release",   "ms"});
    registerParam("makeup_db",    {  0.0f,   0.0f,  24.0f, "Makeup",    "dB"});
    registerParam("knee_db",      {  6.0f,   0.0f,  24.0f, "Knee",      "dB"});
}

void CompressorNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    m_envelope = -96.0f;
    recalcCoeffs();
}

void CompressorNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcCoeffs();
}

void CompressorNode::recalcCoeffs() {
    float makeupDb = getParam("makeup_db");
    m_makeupLin    = std::pow(10.0f, makeupDb / 20.0f);
    m_ratio        = getParam("ratio");
    m_kneeDb       = getParam("knee_db");

    double sr       = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    float attackMs  = getParam("attack_ms");
    float releaseMs = getParam("release_ms");
    m_attackCoeff   = static_cast<float>(std::exp(-1.0 / (sr * attackMs  * 0.001)));
    m_releaseCoeff  = static_cast<float>(std::exp(-1.0 / (sr * releaseMs * 0.001)));
}

// Soft-knee gain computation (in dB domain).
float CompressorNode::computeGainDb(float inputDb) const {
    float threshDb = getParam("threshold_db");
    float diff     = inputDb - threshDb;
    float halfKnee = m_kneeDb * 0.5f;

    float gainDb;
    if (diff < -halfKnee) {
        gainDb = 0.0f;  // below knee: no compression
    } else if (diff < halfKnee) {
        // Within knee: smooth transition
        float x = (diff + halfKnee) / m_kneeDb;
        gainDb  = (1.0f / m_ratio - 1.0f) * x * x * m_kneeDb * 0.5f;
    } else {
        gainDb = (1.0f / m_ratio - 1.0f) * diff;
    }
    return gainDb;
}

void CompressorNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    static constexpr float kEps = 1e-10f;

    for (int s = 0; s < numSamples; ++s) {
        // Sum of squares across channels for level detection
        float sumSq = 0.0f;
        for (int c = 0; c < input.numChannels; ++c) {
            float x = input[c][s];
            sumSq += x * x;
        }
        float rms = std::sqrt(sumSq / std::max(1, input.numChannels));
        float rmsDb = 20.0f * std::log10(rms + kEps);

        // Envelope follower in dB domain
        if (rmsDb > m_envelope)
            m_envelope = rmsDb + m_attackCoeff  * (m_envelope - rmsDb);
        else
            m_envelope = rmsDb + m_releaseCoeff * (m_envelope - rmsDb);

        float gainDb  = computeGainDb(m_envelope);
        float gainLin = std::pow(10.0f, gainDb / 20.0f) * m_makeupLin;

        for (int c = 0; c < output.numChannels; ++c)
            output[c][s] = input[c][s] * gainLin;
    }
}

} // namespace gearboxfx
