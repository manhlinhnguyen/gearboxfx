#include "effects/modulation/PitchShifterNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

static constexpr float kPi = 3.14159265358979f;

PitchShifterNode::PitchShifterNode() {
    registerParam("semitones", {0.0f, -12.0f, 12.0f, "Semitones", "st"});
    registerParam("mix",       {1.0f,  0.0f,  1.0f,  "Mix",       ""});
}

void PitchShifterNode::onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {
    for (int c = 0; c < 2; ++c)
        m_buf[c].assign(kBufSize, 0.0f);
    m_writePos  = 0;
    m_readPos[0] = 0.0f;
    m_readPos[1] = static_cast<float>(kGrainSize) * 0.5f;  // offset by half grain
}

float PitchShifterNode::readHermite(const std::vector<float>& buf, float pos) const {
    int   i  = static_cast<int>(pos);
    float fr = pos - static_cast<float>(i);

    // Wrap indices into buffer
    auto idx = [](int n) { return ((n % kBufSize) + kBufSize) % kBufSize; };
    float y0 = buf[idx(i - 1)];
    float y1 = buf[idx(i    )];
    float y2 = buf[idx(i + 1)];
    float y3 = buf[idx(i + 2)];

    // Hermite cubic
    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
    return ((c3 * fr + c2) * fr + c1) * fr + c0;
}

void PitchShifterNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    float semitones = getParam("semitones");
    float mix       = getParam("mix");

    float ratio   = std::pow(2.0f, semitones / 12.0f);
    float dryGain = 1.0f - mix;
    float wetGain = mix;

    // Hann window function over [0, kGrainSize)
    auto hannWin = [](float pos) -> float {
        float t = pos / static_cast<float>(kGrainSize);
        return 0.5f * (1.0f - std::cos(2.0f * kPi * t));
    };

    for (int s = 0; s < numSamples; ++s) {
        // Write current input into buffer
        for (int c = 0; c < std::min(2, input.numChannels); ++c)
            m_buf[c][m_writePos] = input[c][s];

        float wet = 0.0f;

        for (int head = 0; head < 2; ++head) {
            // Read position in buffer = writePos - (grainSize - readPos)
            float readBufPos = static_cast<float>(m_writePos)
                               - (static_cast<float>(kGrainSize) - m_readPos[head]);
            if (readBufPos < 0.0f) readBufPos += static_cast<float>(kBufSize);

            float w = hannWin(m_readPos[head]);

            // Mix channels for mono grain output; per-channel handled below
            float sample = 0.0f;
            int numCh = std::min(2, input.numChannels);
            for (int c = 0; c < numCh; ++c)
                sample += readHermite(m_buf[c], readBufPos);
            sample /= static_cast<float>(std::max(1, numCh));

            wet += sample * w;

            // Advance read head at pitch ratio
            m_readPos[head] += ratio;
            if (m_readPos[head] >= static_cast<float>(kGrainSize))
                m_readPos[head] -= static_cast<float>(kGrainSize);
        }

        // Normalize: two Hann windows that are half-period offset sum to ~1.0
        // No extra normalization needed (Hann overlap-add property).

        for (int c = 0; c < output.numChannels; ++c)
            output[c][s] = input[c][s] * dryGain + wet * wetGain;

        m_writePos = (m_writePos + 1) % kBufSize;
    }
}

} // namespace gearboxfx
