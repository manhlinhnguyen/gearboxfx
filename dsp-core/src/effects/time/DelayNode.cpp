#include "effects/time/DelayNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

DelayNode::DelayNode() {
    registerParam("time_ms",  {300.0f,   1.0f, 2000.0f, "Time",     "ms"});
    registerParam("feedback", {  0.4f,   0.0f,    0.99f, "Feedback", ""});
    registerParam("mix",      {  0.5f,   0.0f,    1.0f,  "Mix",      ""});
    registerParam("bpm_sync", {  0.0f,   0.0f,    1.0f,  "BPM Sync", ""});
    registerParam("bpm",      {120.0f,  60.0f,  240.0f,  "BPM",      "bpm"});
}

void DelayNode::onPrepare(double sampleRate, int /*maxBlockSize*/) {
    for (int c = 0; c < 2; ++c)
        m_buf[c].assign(kMaxDelaySamples, 0.0f);
    m_writePos = 0;
    recalcDelaySamples();
}

void DelayNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    recalcDelaySamples();
    m_feedback = getParam("feedback");
    m_mix      = getParam("mix");
}

void DelayNode::recalcDelaySamples() {
    double sr  = m_sampleRate > 0 ? m_sampleRate : 48000.0;
    m_bpmSync  = getParam("bpm_sync") > 0.5f;
    m_bpm      = getParam("bpm");

    float timeMs;
    if (m_bpmSync) {
        // Quarter-note delay: 60000/bpm ms
        timeMs = 60000.0f / m_bpm;
    } else {
        timeMs = getParam("time_ms");
    }

    m_delaySamples = timeMs * static_cast<float>(sr) / 1000.0f;
    m_delaySamples = std::max(1.0f, std::min(m_delaySamples, (float)(kMaxDelaySamples - 2)));
}

// Hermite 4-point cubic interpolation
float DelayNode::hermiteRead(const std::vector<float>& buf, float delaySamps) const {
    int   d  = static_cast<int>(delaySamps);
    float fr = delaySamps - static_cast<float>(d);
    int   n  = static_cast<int>(buf.size());

    auto at = [&](int offset) -> float {
        int idx = ((m_writePos - d - offset) % n + n) % n;
        return buf[idx];
    };

    float y0 = at(1);
    float y1 = at(0);
    float y2 = at(-1);
    float y3 = at(-2);

    float c0 = y1;
    float c1 = 0.5f * (y2 - y0);
    float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

    return ((c3 * fr + c2) * fr + c1) * fr + c0;
}

void DelayNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_feedback      = getParam("feedback");
    m_mix           = getParam("mix");
    float dryGain   = 1.0f - m_mix;
    float wetGain   = m_mix;

    for (int s = 0; s < numSamples; ++s) {
        for (int c = 0; c < output.numChannels; ++c) {
            int ch = std::min(c, 1);

            float delayed = hermiteRead(m_buf[ch], m_delaySamples);
            m_buf[ch][m_writePos] = input[c][s] + delayed * m_feedback;
            output[c][s] = input[c][s] * dryGain + delayed * wetGain;
        }

        m_writePos = (m_writePos + 1) % kMaxDelaySamples;
    }
}

} // namespace gearboxfx
