#include <gtest/gtest.h>
#include "PresetStore.h"
#include "EffectChain.h"
#include "effects/EffectNodeRegistry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using namespace gearboxfx;

static constexpr double kSR    = 48000.0;
static constexpr int    kBlock = 256;

class PresetStoreTest : public ::testing::Test {
protected:
    EffectNodeRegistry reg;
    EffectChain        chain;

    void SetUp() override {
        chain.prepare(kSR, kBlock);
    }
};

TEST_F(PresetStoreTest, LoadCleanBoostPreset) {
    auto result = PresetStore::loadFromFile(
        "presets/01_clean_boost.json", chain, reg, kSR, kBlock);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Clean Boost");
    EXPECT_EQ(result->routingMode, "serial");
    EXPECT_FALSE(chain.nodes().empty());
}

TEST_F(PresetStoreTest, LoadAllPresets) {
    std::vector<std::string> presets = {
        "presets/01_clean_boost.json",
        "presets/02_mild_overdrive.json",
        "presets/03_heavy_distortion.json",
        "presets/04_chorus_clean.json",
        "presets/05_delay_reverb.json"
    };

    for (auto& path : presets) {
        EffectChain localChain;
        localChain.prepare(kSR, kBlock);
        auto result = PresetStore::loadFromFile(path, localChain, reg, kSR, kBlock);
        EXPECT_TRUE(result.has_value()) << "Failed to load: " << path;
        EXPECT_FALSE(localChain.nodes().empty()) << "Empty chain for: " << path;
    }
}

TEST_F(PresetStoreTest, LoadFromJsonRoundTrip) {
    nlohmann::json j = {
        {"preset_id",      "test-uuid"},
        {"format_version", "1.0"},
        {"name",           "Test Preset"},
        {"routing_mode",   "serial"},
        {"output_volume",  0.9f},
        {"output_eq",      {{"bass_db", 1.0f}, {"mid_db", 0.0f}, {"treble_db", -1.0f}}},
        {"effect_chain", {
            {
                {"id",      "boost_test"},
                {"type",    "gain.clean_boost"},
                {"enabled", true},
                {"params",  {{"gain_db", 3.0f}}}
            }
        }}
    };

    auto result = PresetStore::loadFromJson(j, chain, reg, kSR, kBlock);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Test Preset");
    EXPECT_EQ(chain.nodes().size(), 1u);

    auto node = chain.findNode("boost_test");
    ASSERT_NE(node, nullptr);
    EXPECT_NEAR(node->getParam("gain_db"), 3.0f, 0.001f);
}

TEST_F(PresetStoreTest, SaveAndReload) {
    // Load a preset first
    auto result = PresetStore::loadFromFile(
        "presets/02_mild_overdrive.json", chain, reg, kSR, kBlock);
    ASSERT_TRUE(result.has_value());

    // Save to a temp file
    std::string tempPath = "presets/test_save_output.json";
    bool saved = PresetStore::saveToFile(tempPath, *result, chain);
    EXPECT_TRUE(saved);

    // Reload and compare node count
    EffectChain chain2;
    chain2.prepare(kSR, kBlock);
    auto result2 = PresetStore::loadFromFile(tempPath, chain2, reg, kSR, kBlock);
    ASSERT_TRUE(result2.has_value());

    EXPECT_EQ(chain2.nodes().size(), chain.nodes().size());
    EXPECT_EQ(result2->name, result->name);

    // Cleanup
    std::filesystem::remove(tempPath);
}

TEST_F(PresetStoreTest, MissingFileReturnsNullopt) {
    auto result = PresetStore::loadFromFile(
        "presets/nonexistent.json", chain, reg, kSR, kBlock);
    EXPECT_FALSE(result.has_value());
}

TEST_F(PresetStoreTest, UnknownEffectTypeIsSkipped) {
    nlohmann::json j = {
        {"preset_id",      "test"},
        {"format_version", "1.0"},
        {"name",           "Test"},
        {"routing_mode",   "serial"},
        {"output_volume",  1.0f},
        {"effect_chain", {
            {
                {"id",      "known_1"},
                {"type",    "gain.clean_boost"},
                {"enabled", true},
                {"params",  {{"gain_db", 0.0f}}}
            },
            {
                {"id",      "unknown_1"},
                {"type",    "future.new_effect"},
                {"enabled", true},
                {"params",  {}}
            }
        }}
    };

    auto result = PresetStore::loadFromJson(j, chain, reg, kSR, kBlock);
    ASSERT_TRUE(result.has_value());
    // Only the known node should be in the chain
    EXPECT_EQ(chain.nodes().size(), 1u);
}

TEST_F(PresetStoreTest, ParamBoundsAreClamped) {
    nlohmann::json j = {
        {"preset_id",      "clamp-test"},
        {"format_version", "1.0"},
        {"name",           "Clamp Test"},
        {"routing_mode",   "serial"},
        {"output_volume",  1.0f},
        {"effect_chain", {
            {
                {"id",      "boost_1"},
                {"type",    "gain.clean_boost"},
                {"enabled", true},
                {"params",  {{"gain_db", 999.0f}}}  // way above max of 20 dB
            }
        }}
    };

    auto result = PresetStore::loadFromJson(j, chain, reg, kSR, kBlock);
    ASSERT_TRUE(result.has_value());

    auto node = chain.findNode("boost_1");
    ASSERT_NE(node, nullptr);
    EXPECT_LE(node->getParam("gain_db"), 20.0f);  // should be clamped
}

TEST_F(PresetStoreTest, DisabledNodeIsPreserved) {
    nlohmann::json j = {
        {"preset_id",      "disable-test"},
        {"format_version", "1.0"},
        {"name",           "Disable Test"},
        {"routing_mode",   "serial"},
        {"output_volume",  1.0f},
        {"effect_chain", {
            {
                {"id",      "chorus_1"},
                {"type",    "modulation.chorus"},
                {"enabled", false},
                {"params",  {}}
            }
        }}
    };

    auto result = PresetStore::loadFromJson(j, chain, reg, kSR, kBlock);
    ASSERT_TRUE(result.has_value());

    auto node = chain.findNode("chorus_1");
    ASSERT_NE(node, nullptr);
    EXPECT_FALSE(node->isEnabled());
}
