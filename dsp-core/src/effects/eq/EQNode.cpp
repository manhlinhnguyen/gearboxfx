#include "effects/eq/EQNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kPi = 3.14159265358979f;

EQNode::EQNode() {
    registerParam("bass_db",   {0.0f, -12.0f, 12.0f, "Bass",     "dB"});
    registerParam("mid_db",    {0.0f, -12.0f, 12.0f, "Mid",      "dB"});
    registerParam("treble_db", {0.0f, -12.0f, 12.0f, "Treble",   "dB"});
    registerParam("mid_freq",  {800.0f, 200.0f, 5000.0f, "Mid Freq", "Hz"});
}

void EQNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    for (int b = 0; b < 3; ++b)
        for (int c = 0; c < 2; ++c)
            m_state[b][c] = {};
    recalcCoeffs();
}

void EQNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcCoeffs();
}

void EQNode::recalcCoeffs() {
    float sr       = static_cast<float>(m_sampleRate > 0 ? m_sampleRate : 48000.0);
    float bassDb   = getParam("bass_db");
    float midDb    = getParam("mid_db");
    float trebleDb = getParam("treble_db");
    float midFreq  = getParam("mid_freq");

    m_coeffs[0] = makeLowShelf (bassDb,   80.0f,   sr);
    m_coeffs[1] = makePeaking  (midDb,    midFreq, sr, 1.0f);
    m_coeffs[2] = makeHighShelf(trebleDb, 8000.0f, sr);
}

// Audio EQ Cookbook — low-shelf (S = 1 for maximum slope)
EQNode::BiquadCoeffs EQNode::makeLowShelf(float gainDb, float freqHz, float sr) {
    float A  = std::pow(10.0f, gainDb / 40.0f);
    float w0 = 2.0f * kPi * freqHz / sr;
    float cosW = std::cos(w0);
    float sinW = std::sin(w0);
    // alpha with shelf-slope S=1: α = sin(w0)/2 * sqrt((A + 1/A)*(1/S - 1) + 2) → simplifies to sin(w0)*sqrt(A)
    float alpha = sinW * std::sqrt(A);
    float sqA2  = 2.0f * std::sqrt(A);

    float a0 = (A + 1) + (A - 1) * cosW + sqA2 * alpha;
    BiquadCoeffs c;
    c.b0 = A * ((A + 1) - (A - 1) * cosW + sqA2 * alpha) / a0;
    c.b1 = A * ( 2.0f  * ((A - 1) - (A + 1) * cosW)    ) / a0;
    c.b2 = A * ((A + 1) - (A - 1) * cosW - sqA2 * alpha) / a0;
    c.a1 =    (-2.0f  * ((A - 1) + (A + 1) * cosW)    )  / a0;
    c.a2 =    ( (A + 1) + (A - 1) * cosW - sqA2 * alpha) / a0;
    return c;
}

// Audio EQ Cookbook — high-shelf (S = 1)
EQNode::BiquadCoeffs EQNode::makeHighShelf(float gainDb, float freqHz, float sr) {
    float A  = std::pow(10.0f, gainDb / 40.0f);
    float w0 = 2.0f * kPi * freqHz / sr;
    float cosW = std::cos(w0);
    float sinW = std::sin(w0);
    float alpha = sinW * std::sqrt(A);
    float sqA2  = 2.0f * std::sqrt(A);

    float a0 = (A + 1) - (A - 1) * cosW + sqA2 * alpha;
    BiquadCoeffs c;
    c.b0 = A * ((A + 1) + (A - 1) * cosW + sqA2 * alpha) / a0;
    c.b1 = A * (-2.0f  * ((A - 1) + (A + 1) * cosW)    ) / a0;
    c.b2 = A * ((A + 1) + (A - 1) * cosW - sqA2 * alpha) / a0;
    c.a1 =    ( 2.0f   * ((A - 1) - (A + 1) * cosW)    )  / a0;
    c.a2 =    ( (A + 1) - (A - 1) * cosW - sqA2 * alpha)  / a0;
    return c;
}

// Audio EQ Cookbook — peaking EQ filter
EQNode::BiquadCoeffs EQNode::makePeaking(float gainDb, float freqHz, float sr, float Q) {
    float A  = std::pow(10.0f, gainDb / 40.0f);
    float w0 = 2.0f * kPi * freqHz / sr;
    float alpha = std::sin(w0) / (2.0f * Q);

    float a0 = 1.0f + alpha / A;
    BiquadCoeffs c;
    c.b0 = (1.0f + alpha * A) / a0;
    c.b1 = (-2.0f * std::cos(w0)) / a0;
    c.b2 = (1.0f - alpha * A) / a0;
    c.a1 = c.b1;  // same as b1/a0 already normalized
    c.a2 = (1.0f - alpha / A) / a0;
    return c;
}

float EQNode::processBiquad(float x, BiquadState& s, const BiquadCoeffs& c) {
    float y = c.b0 * x + c.b1 * s.x1 + c.b2 * s.x2
                       - c.a1 * s.y1 - c.a2 * s.y2;
    s.x2 = s.x1;  s.x1 = x;
    s.y2 = s.y1;  s.y1 = y;
    return y;
}

void EQNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    for (int s = 0; s < numSamples; ++s) {
        for (int c = 0; c < output.numChannels; ++c) {
            int ch = (c < 2) ? c : 1;
            float x = input[c][s];
            // Chain 3 bands
            x = processBiquad(x, m_state[0][ch], m_coeffs[0]);
            x = processBiquad(x, m_state[1][ch], m_coeffs[1]);
            x = processBiquad(x, m_state[2][ch], m_coeffs[2]);
            output[c][s] = x;
        }
    }
}

} // namespace gearboxfx
