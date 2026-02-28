#include <gtest/gtest.h>
#include "effects/EffectNodeRegistry.h"
#include "AudioBuffer.h"
#include <cmath>

using namespace gearboxfx;

static constexpr double kSR       = 48000.0;
static constexpr int    kBlock    = 256;
static constexpr int    kCh       = 2;

static AudioBuffer makeSilence() {
    return AudioBuffer(kCh, kBlock);
}

static AudioBuffer makeTone(float freq, float amp = 0.5f) {
    AudioBuffer buf(kCh, kBlock);
    for (int c = 0; c < kCh; ++c)
        for (int s = 0; s < kBlock; ++s)
            buf.getWritePointer(c)[s] = amp * std::sin(2.0f * 3.14159f * freq * s / (float)kSR);
    return buf;
}

static float rms(const float* d, int n) {
    float s = 0; for (int i = 0; i < n; ++i) s += d[i]*d[i];
    return std::sqrt(s / n);
}

static std::shared_ptr<EffectNode> makeNode(const std::string& type) {
    static EffectNodeRegistry reg;
    auto n = reg.create(type);
    EXPECT_NE(n, nullptr) << "type: " << type;
    n->setId(type + "_test");
    n->prepare(kSR, kBlock);
    return n;
}

// ── Silence in → silence out ─────────────────────────────────────────────────

#define TEST_SILENCE_PASSTHROUGH(TestName, TypeStr) \
    TEST(Effects, TestName##_SilenceInSilenceOut) { \
        auto node = makeNode(TypeStr); \
        AudioBuffer in = makeSilence(), out(kCh, kBlock); \
        auto iv = in.view(), ov = out.view(); \
        /* run a few blocks so state settles */ \
        for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock); \
        EXPECT_LT(rms(out.getReadPointer(0), kBlock), 1e-5f); \
    }

TEST_SILENCE_PASSTHROUGH(dynamics_noise_gate,          "dynamics.noise_gate")
TEST_SILENCE_PASSTHROUGH(dynamics_compressor,          "dynamics.compressor")
TEST_SILENCE_PASSTHROUGH(eq_parametric,                "eq.parametric")
TEST_SILENCE_PASSTHROUGH(gain_clean_boost,             "gain.clean_boost")
TEST_SILENCE_PASSTHROUGH(gain_overdrive,               "gain.overdrive")
TEST_SILENCE_PASSTHROUGH(gain_distortion,              "gain.distortion")
TEST_SILENCE_PASSTHROUGH(modulation_chorus,            "modulation.chorus")
TEST_SILENCE_PASSTHROUGH(modulation_flanger,           "modulation.flanger")
TEST_SILENCE_PASSTHROUGH(modulation_phaser,            "modulation.phaser")
TEST_SILENCE_PASSTHROUGH(modulation_pitch_shifter,     "modulation.pitch_shifter")
TEST_SILENCE_PASSTHROUGH(modulation_tremolo,           "modulation.tremolo")
TEST_SILENCE_PASSTHROUGH(output_volume,                "output.volume")
TEST_SILENCE_PASSTHROUGH(time_delay,                   "time.delay")
TEST_SILENCE_PASSTHROUGH(time_reverb,                  "time.reverb")

// ── Signal level tests ────────────────────────────────────────────────────────

TEST(Effects, CleanBoost_GainPositive) {
    auto node = makeNode("gain.clean_boost");
    node->setParam("gain_db", 6.0f);  // ~2x

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut / rmsIn, std::pow(10.0f, 6.0f / 20.0f), 0.05f);
}

TEST(Effects, CleanBoost_GainNegative) {
    auto node = makeNode("gain.clean_boost");
    node->setParam("gain_db", -6.0f);  // ~0.5x

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut / rmsIn, std::pow(10.0f, -6.0f / 20.0f), 0.05f);
}

TEST(Effects, NoiseGate_BlocksLowSignal) {
    auto node = makeNode("dynamics.noise_gate");
    node->setParam("threshold_db", -40.0f);  // threshold = ~0.01 linear
    node->setParam("release_ms", 10.0f);

    // Very quiet signal (below threshold)
    AudioBuffer in  = makeTone(440.0f, 0.001f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Run many blocks to let gate settle to closed
    for (int i = 0; i < 50; ++i) node->process(iv, ov, kBlock);

    EXPECT_LT(rms(out.getReadPointer(0), kBlock), 0.0005f);
}

TEST(Effects, NoiseGate_PassesLoudSignal) {
    auto node = makeNode("dynamics.noise_gate");
    node->setParam("threshold_db", -60.0f);  // very low threshold

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Warm up gate
    for (int i = 0; i < 5; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_GT(rmsOut / rmsIn, 0.9f);  // should mostly pass through
}

TEST(Effects, Overdrive_SaturatesHighGain) {
    auto node = makeNode("gain.overdrive");
    node->setParam("gain", 1.0f);
    node->setParam("level", 1.0f);
    node->setParam("tone", 0.5f);

    // Loud input that should clip
    AudioBuffer in  = makeTone(440.0f, 0.9f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    // Output should be bounded (arctan clips at ±1)
    const float* o = out.getReadPointer(0);
    bool allBounded = true;
    for (int s = 0; s < kBlock; ++s)
        if (std::abs(o[s]) > 1.01f) allBounded = false;
    EXPECT_TRUE(allBounded);
}

TEST(Effects, Distortion_HardClipsBounded) {
    auto node = makeNode("gain.distortion");
    node->setParam("gain", 1.0f);
    node->setParam("level", 1.0f);

    AudioBuffer in  = makeTone(440.0f, 1.0f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    const float* o = out.getReadPointer(0);
    for (int s = 0; s < kBlock; ++s)
        EXPECT_LE(std::abs(o[s]), 1.1f);  // should be clipped
}

TEST(Effects, Compressor_ReducesDynamics) {
    auto node = makeNode("dynamics.compressor");
    node->setParam("threshold_db", -20.0f);
    node->setParam("ratio", 10.0f);
    node->setParam("makeup_db", 0.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Warm up
    for (int i = 0; i < 20; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);

    // With high ratio and loud input, output should be significantly quieter
    EXPECT_LT(rmsOut, rmsIn);
}

TEST(Effects, Delay_ProducesDelayedSignal) {
    auto node = makeNode("time.delay");
    node->setParam("time_ms", 100.0f);
    node->setParam("feedback", 0.0f);
    node->setParam("mix", 1.0f);  // 100% wet

    // Process a short impulse followed by silence
    AudioBuffer impulse(kCh, kBlock);
    impulse.getWritePointer(0)[0] = 1.0f;
    impulse.getWritePointer(1)[0] = 1.0f;

    AudioBuffer out(kCh, kBlock);
    auto iv = impulse.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    // With 100% wet and 100ms delay, first block output should be near silence
    // (delayed signal hasn't arrived yet within the same block)
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_LT(rmsOut, 0.1f);
}

TEST(Effects, Reverb_ProducesWetSignal) {
    auto node = makeNode("time.reverb");
    node->setParam("mix", 1.0f);  // 100% wet
    node->setParam("size", 0.5f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Run many blocks to fill reverb buffers
    for (int i = 0; i < 20; ++i) node->process(iv, ov, kBlock);

    // Output should have signal
    EXPECT_GT(rms(out.getReadPointer(0), kBlock), 0.001f);
}

TEST(Effects, Chorus_MixBlendsDryWet) {
    auto node = makeNode("modulation.chorus");
    node->setParam("mix", 0.0f);  // 100% dry

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.01f);  // dry = input
}

TEST(Effects, Tremolo_ModulatesAmplitude) {
    auto node = makeNode("modulation.tremolo");
    node->setParam("rate", 10.0f);
    node->setParam("depth", 1.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    // With full depth tremolo, signal should vary — check it's not same as input
    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_LT(rmsOut, rmsIn);  // tremolo reduces average level
}

TEST(Effects, Registry_AllTypesRegistered) {
    EffectNodeRegistry reg;
    std::vector<std::string> expected = {
        "dynamics.noise_gate", "dynamics.compressor",
        "eq.parametric",
        "gain.clean_boost", "gain.overdrive", "gain.distortion",
        "modulation.chorus", "modulation.flanger", "modulation.phaser",
        "modulation.pitch_shifter", "modulation.tremolo",
        "output.volume",
        "time.delay", "time.reverb"
    };
    for (auto& t : expected) {
        EXPECT_TRUE(reg.has(t)) << "Missing: " << t;
        auto node = reg.create(t);
        EXPECT_NE(node, nullptr) << "Cannot create: " << t;
    }
}

// ── EQ (eq.parametric) ────────────────────────────────────────────────────────

TEST(Effects, EQ_FlatSettingIsTransparent) {
    // Default params are all 0 dB → biquads collapse to identity (H(z)=1)
    auto node = makeNode("eq.parametric");
    // defaults already 0 dB — no setParam needed

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.01f);  // ≤ 1% deviation
}

TEST(Effects, EQ_TrebleBoostIncreasesHFRMS) {
    auto node = makeNode("eq.parametric");
    node->setParam("treble_db", 12.0f);

    // 12 kHz tone — well inside the high-shelf boost region (shelf @ 8 kHz)
    AudioBuffer in  = makeTone(12000.0f, 0.3f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_GT(rmsOut, rmsIn * 1.2f);  // must be noticeably louder
}

TEST(Effects, EQ_BassCutReducesLFRMS) {
    auto node = makeNode("eq.parametric");
    node->setParam("bass_db", -12.0f);

    // 60 Hz tone — well inside the low-shelf cut region (shelf @ 80 Hz)
    AudioBuffer in  = makeTone(60.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 20; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_LT(rmsOut, rmsIn * 0.8f);  // must be noticeably quieter
}

TEST(Effects, EQ_MidBoostIncreasesMFRMS) {
    auto node = makeNode("eq.parametric");
    node->setParam("mid_db",   12.0f);
    node->setParam("mid_freq", 1000.0f);

    // 1 kHz tone — directly at the peak center
    AudioBuffer in  = makeTone(1000.0f, 0.3f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_GT(rmsOut, rmsIn * 1.2f);
}

// ── Flanger (modulation.flanger) ──────────────────────────────────────────────

TEST(Effects, Flanger_DryMixPassesThrough) {
    auto node = makeNode("modulation.flanger");
    node->setParam("mix", 0.0f);  // 100% dry

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.01f);
}

TEST(Effects, Flanger_WetMixProducesSignal) {
    auto node = makeNode("modulation.flanger");
    node->setParam("mix", 1.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    // Fill delay buffer first
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    // Wet output should carry signal (delayed copy of input)
    EXPECT_GT(rms(out.getReadPointer(0), kBlock), 0.01f);
}

TEST(Effects, Flanger_HighFeedbackStaysBounded) {
    auto node = makeNode("modulation.flanger");
    node->setParam("feedback", 0.9f);
    node->setParam("mix",      0.8f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 200; ++i) node->process(iv, ov, kBlock);

    const float* o = out.getReadPointer(0);
    for (int s = 0; s < kBlock; ++s)
        EXPECT_LT(std::abs(o[s]), 5.0f) << "sample " << s << " diverged";
}

// ── Phaser (modulation.phaser) ────────────────────────────────────────────────

TEST(Effects, Phaser_DryMixPassesThrough) {
    auto node = makeNode("modulation.phaser");
    node->setParam("mix", 0.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.01f);
}

TEST(Effects, Phaser_WetMixProducesSignal) {
    auto node = makeNode("modulation.phaser");
    node->setParam("mix", 1.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 5; ++i) node->process(iv, ov, kBlock);

    // All-pass preserves magnitude, so wet output should have similar energy to input
    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_GT(rmsOut, rmsIn * 0.1f);
}

TEST(Effects, Phaser_HighFeedbackStaysBounded) {
    auto node = makeNode("modulation.phaser");
    node->setParam("feedback", 0.85f);
    node->setParam("mix",      0.8f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 200; ++i) node->process(iv, ov, kBlock);

    const float* o = out.getReadPointer(0);
    for (int s = 0; s < kBlock; ++s)
        EXPECT_LT(std::abs(o[s]), 5.0f) << "sample " << s << " diverged";
}

// ── PitchShifter (modulation.pitch_shifter) ───────────────────────────────────

TEST(Effects, PitchShifter_DryMixPassesThrough) {
    auto node = makeNode("modulation.pitch_shifter");
    node->setParam("mix", 0.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.01f);
}

TEST(Effects, PitchShifter_OctaveUpProducesSignal) {
    auto node = makeNode("modulation.pitch_shifter");
    node->setParam("semitones", 12.0f);
    node->setParam("mix",       1.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Fill the grain buffer (2048 samples = 8 blocks) then check output has signal
    for (int i = 0; i < 16; ++i) node->process(iv, ov, kBlock);

    EXPECT_GT(rms(out.getReadPointer(0), kBlock), 0.01f);
}

TEST(Effects, PitchShifter_OutputBounded) {
    auto node = makeNode("modulation.pitch_shifter");
    node->setParam("semitones", 7.0f);
    node->setParam("mix",       1.0f);

    AudioBuffer in  = makeTone(440.0f, 0.9f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 100; ++i) node->process(iv, ov, kBlock);

    // Hann overlap-add: two windows sum to ≤ 1, so output ≤ input amplitude
    const float* o = out.getReadPointer(0);
    for (int s = 0; s < kBlock; ++s)
        EXPECT_LT(std::abs(o[s]), 2.0f) << "sample " << s << " out of range";
}

// ── VolumeNode (output.volume) ────────────────────────────────────────────────

TEST(Effects, Volume_ZeroDbIsTransparent) {
    auto node = makeNode("output.volume");
    // Defaults: volume_db=0, limiter_threshold_db=0 → no gain, limiter at unity

    AudioBuffer in  = makeTone(440.0f, 0.3f);  // below limiter threshold
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_NEAR(rmsOut, rmsIn, rmsIn * 0.02f);
}

TEST(Effects, Volume_BoostIncreasesLevel) {
    auto node = makeNode("output.volume");
    node->setParam("volume_db",            6.0f);
    node->setParam("limiter_threshold_db", 0.0f);  // limiter at unity (won't trigger)

    AudioBuffer in  = makeTone(440.0f, 0.2f);  // quiet enough that 6dB boost stays below 1.0
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    float expected = rmsIn * std::pow(10.0f, 6.0f / 20.0f);
    EXPECT_NEAR(rmsOut, expected, expected * 0.05f);
}

TEST(Effects, Volume_AttenuationReducesLevel) {
    auto node = makeNode("output.volume");
    node->setParam("volume_db",            -6.0f);
    node->setParam("limiter_threshold_db", 0.0f);

    AudioBuffer in  = makeTone(440.0f, 0.5f);
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();
    for (int i = 0; i < 10; ++i) node->process(iv, ov, kBlock);

    float rmsIn  = rms(in.getReadPointer(0), kBlock);
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    float expected = rmsIn * std::pow(10.0f, -6.0f / 20.0f);
    EXPECT_NEAR(rmsOut, expected, expected * 0.05f);
}

TEST(Effects, Volume_LimiterConstrainsHighGain) {
    auto node = makeNode("output.volume");
    node->setParam("volume_db",            12.0f);  // 4x linear gain
    node->setParam("limiter_threshold_db", -6.0f);  // clamp at 0.5 linear

    AudioBuffer in  = makeTone(440.0f, 0.5f);  // peak ≈ 0.5; after gain ≈ 2.0 → limiter kicks in
    AudioBuffer out(kCh, kBlock);
    auto iv = in.view(), ov = out.view();

    // Run enough blocks for envelope follower to settle
    for (int i = 0; i < 50; ++i) node->process(iv, ov, kBlock);

    float threshold = std::pow(10.0f, -6.0f / 20.0f);  // ≈ 0.501
    float rmsOut = rms(out.getReadPointer(0), kBlock);
    EXPECT_LT(rmsOut, threshold * 1.2f);  // should be well below ungained 2.0
}
