#include <gtest/gtest.h>
#include "EffectChain.h"
#include "EffectEngine.h"
#include "AudioBuffer.h"
#include "effects/EffectNodeRegistry.h"
#include <cmath>
#include <numeric>

using namespace gearboxfx;

// ── Helpers ─────────────────────────────────────────────────────────────────

static AudioBuffer makeSilence(int numCh, int numSamples) {
    return AudioBuffer(numCh, numSamples);
}

static AudioBuffer makeTone(int numCh, int numSamples, float freq, float sr) {
    AudioBuffer buf(numCh, numSamples);
    for (int c = 0; c < numCh; ++c)
        for (int s = 0; s < numSamples; ++s)
            buf.getWritePointer(c)[s] = 0.5f * std::sin(2.0f * 3.14159f * freq * s / sr);
    return buf;
}

static float rms(const float* data, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) sum += data[i] * data[i];
    return std::sqrt(sum / n);
}

// ── Tests ────────────────────────────────────────────────────────────────────

TEST(EffectChain, EmptyChainPassesThrough) {
    EffectChain chain;
    chain.prepare(48000.0, 256);

    AudioBuffer in  = makeTone(2, 256, 440.0f, 48000.0f);
    AudioBuffer out(2, 256);

    auto iv = in.view(), ov = out.view();
    chain.process(iv, ov, 256);

    for (int s = 0; s < 256; ++s) {
        EXPECT_FLOAT_EQ(out.getReadPointer(0)[s], in.getReadPointer(0)[s]);
        EXPECT_FLOAT_EQ(out.getReadPointer(1)[s], in.getReadPointer(1)[s]);
    }
}

TEST(EffectChain, SilenceInSilenceOut) {
    EffectNodeRegistry reg;
    EffectChain chain;

    auto ng = reg.create("dynamics.noise_gate");
    ng->setId("ng_1");
    chain.prepare(48000.0, 256);
    chain.addNode(ng);

    AudioBuffer in  = makeSilence(2, 256);
    AudioBuffer out(2, 256);

    auto iv = in.view(), ov = out.view();
    chain.process(iv, ov, 256);

    EXPECT_LT(rms(out.getReadPointer(0), 256), 1e-6f);
    EXPECT_LT(rms(out.getReadPointer(1), 256), 1e-6f);
}

TEST(EffectChain, BypassedNodePassesThrough) {
    EffectNodeRegistry reg;
    EffectChain chain;

    auto node = reg.create("gain.distortion");
    node->setId("dist_1");
    node->setEnabled(false);   // bypass this node

    chain.prepare(48000.0, 256);
    chain.addNode(node);

    AudioBuffer in  = makeTone(2, 256, 440.0f, 48000.0f);
    AudioBuffer out(2, 256);
    auto iv = in.view(), ov = out.view();
    chain.process(iv, ov, 256);

    for (int s = 0; s < 256; ++s)
        EXPECT_FLOAT_EQ(out.getReadPointer(0)[s], in.getReadPointer(0)[s]);
}

TEST(EffectChain, AddRemoveNode) {
    EffectNodeRegistry reg;
    EffectChain chain;
    chain.prepare(48000.0, 256);

    auto node = reg.create("gain.clean_boost");
    node->setId("boost_1");
    chain.addNode(node);

    EXPECT_EQ(chain.nodes().size(), 1u);

    bool removed = chain.removeNode("boost_1");
    EXPECT_TRUE(removed);
    EXPECT_EQ(chain.nodes().size(), 0u);

    bool notFound = chain.removeNode("nonexistent");
    EXPECT_FALSE(notFound);
}

TEST(EffectChain, MultipleNodesProcessed) {
    EffectNodeRegistry reg;
    EffectChain chain;
    chain.prepare(48000.0, 256);

    auto boost = reg.create("gain.clean_boost");
    boost->setId("boost_1");
    boost->setParam("gain_db", 6.0f);  // ~2x gain

    auto boost2 = reg.create("gain.clean_boost");
    boost2->setId("boost_2");
    boost2->setParam("gain_db", 6.0f);  // another ~2x

    chain.addNode(boost);
    chain.addNode(boost2);

    AudioBuffer in  = makeTone(2, 256, 440.0f, 48000.0f);
    AudioBuffer out(2, 256);
    auto iv = in.view(), ov = out.view();
    chain.process(iv, ov, 256);

    // Output should be ~4x the input amplitude
    float rmsIn  = rms(in.getReadPointer(0), 256);
    float rmsOut = rms(out.getReadPointer(0), 256);
    EXPECT_NEAR(rmsOut / rmsIn, 4.0f, 0.5f);  // ~4x with some tolerance
}

TEST(EffectEngine, BypassPassesThrough) {
    EffectEngine engine;
    engine.prepare(48000.0, 256);
    engine.setBypass(true);

    AudioBuffer in  = makeTone(2, 256, 440.0f, 48000.0f);
    AudioBuffer out(2, 256);
    auto iv = in.view(), ov = out.view();
    engine.processBlock(iv, ov, 256);

    for (int s = 0; s < 256; ++s)
        EXPECT_FLOAT_EQ(out.getReadPointer(0)[s], in.getReadPointer(0)[s]);
}

TEST(EffectEngine, LoadPreset) {
    EffectEngine engine;
    engine.prepare(48000.0, 256);

    bool ok = engine.loadPreset("presets/01_clean_boost.json");
    EXPECT_TRUE(ok);
    EXPECT_FALSE(engine.chain().nodes().empty());
}
