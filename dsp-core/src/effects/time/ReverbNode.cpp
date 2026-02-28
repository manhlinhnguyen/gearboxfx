#include "effects/time/ReverbNode.h"
#include <cmath>
#include <algorithm>

namespace gearboxfx {

// ── CombFilter ─────────────────────────────────────────────────────────────
float ReverbNode::CombFilter::process(float x) {
    float out  = buf[pos];
    store      = out * (1.0f - damp) + store * damp;  // damping lowpass
    buf[pos]   = x + store * feedback;
    pos        = (pos + 1) % static_cast<int>(buf.size());
    return out;
}

// ── AllpassFilter ──────────────────────────────────────────────────────────
float ReverbNode::AllpassFilter::process(float x) {
    float buffered = buf[pos];
    float out      = buffered - x;
    buf[pos]       = x + buffered * feedback;
    pos            = (pos + 1) % static_cast<int>(buf.size());
    return out;
}

// ── ReverbNode ─────────────────────────────────────────────────────────────
ReverbNode::ReverbNode() {
    registerParam("size",         {0.5f, 0.0f, 1.0f,  "Size",       ""});
    registerParam("decay",        {0.5f, 0.0f, 1.0f,  "Decay",      ""});
    registerParam("damping",      {0.5f, 0.0f, 1.0f,  "Damping",    ""});
    registerParam("pre_delay_ms", {10.0f,0.0f,100.0f, "Pre-Delay",  "ms"});
    registerParam("mix",          {0.3f, 0.0f, 1.0f,  "Mix",        ""});
}

void ReverbNode::onPrepare(double sampleRate, int /*maxBlockSize*/) {
    m_preDelayBuf[0].assign(static_cast<int>(sampleRate * 0.2), 0.0f);
    m_preDelayBuf[1].assign(static_cast<int>(sampleRate * 0.2), 0.0f);
    m_preDelayPos = 0;
    rebuildFilters();
}

void ReverbNode::onParamChanged(const std::string& /*name*/, float /*value*/) {
    rebuildFilters();
}

void ReverbNode::rebuildFilters() {
    double sr = m_sampleRate > 0 ? m_sampleRate : 48000.0;

    m_size       = getParam("size");
    m_decay      = getParam("decay");
    m_damping    = getParam("damping");
    m_preDelayMs = getParam("pre_delay_ms");
    m_mix        = getParam("mix");

    m_preDelaySamples = static_cast<int>(m_preDelayMs * sr / 1000.0);
    m_preDelaySamples = std::max(0, std::min(m_preDelaySamples,
        (int)m_preDelayBuf[0].size() - 1));

    float sizeScale = 0.5f + m_size * 1.0f;  // [0.5, 1.5]
    float feedbackBase = 0.6f + m_decay * 0.35f;  // [0.6, 0.95]

    for (int ch = 0; ch < 2; ++ch) {
        for (int i = 0; i < kNumCombs; ++i) {
            int len = static_cast<int>(kCombBaseMs[i] * sizeScale * sr / 1000.0);
            len = std::max(len, 64);
            m_combs[ch][i].resize(len);
            m_combs[ch][i].feedback = feedbackBase;
            m_combs[ch][i].damp     = m_damping * 0.5f;
        }
        for (int i = 0; i < kNumAllpass; ++i) {
            int len = static_cast<int>(kAllpassBaseMs[i] * sizeScale * sr / 1000.0);
            len = std::max(len, 8);
            m_allpass[ch][i].resize(len);
            m_allpass[ch][i].feedback = 0.5f;
        }
    }
}

void ReverbNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    m_mix = getParam("mix");
    float dryGain = 1.0f - m_mix;
    float wetGain = m_mix;

    for (int s = 0; s < numSamples; ++s) {
        for (int ch = 0; ch < std::min(2, output.numChannels); ++ch) {
            int c = std::min(ch, input.numChannels - 1);

            // Pre-delay
            m_preDelayBuf[ch][m_preDelayPos] = input[c][s];
            int readPos = (m_preDelayPos - m_preDelaySamples
                + static_cast<int>(m_preDelayBuf[ch].size()))
                % static_cast<int>(m_preDelayBuf[ch].size());
            float preDel = m_preDelayBuf[ch][readPos];

            // 4 parallel comb filters
            float wet = 0.0f;
            for (int i = 0; i < kNumCombs; ++i)
                wet += m_combs[ch][i].process(preDel);
            wet /= static_cast<float>(kNumCombs);

            // 2 series allpass filters
            for (int i = 0; i < kNumAllpass; ++i)
                wet = m_allpass[ch][i].process(wet);

            output[ch][s] = input[c][s] * dryGain + wet * wetGain;
        }

        // Handle mono input → stereo output
        if (input.numChannels == 1 && output.numChannels == 2)
            output[1][s] = output[0][s];

        m_preDelayPos = (m_preDelayPos + 1) % static_cast<int>(m_preDelayBuf[0].size());
    }
}

} // namespace gearboxfx
