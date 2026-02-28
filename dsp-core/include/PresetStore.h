#pragma once
#include "EffectChain.h"
#include <nlohmann/json.hpp>
#include <string>
#include <optional>

namespace gearboxfx {

// Preset metadata + full JSON payload.
struct Preset {
    std::string         presetId;
    std::string         formatVersion;
    std::string         name;
    std::string         routingMode;  // "serial" for now
    nlohmann::json      raw;          // full parsed JSON for reference

    // Output EQ parameters
    float bassDb    = 0.0f;
    float midDb     = 0.0f;
    float trebleDb  = 0.0f;
    float outputVolume = 0.85f;
};

class EffectNodeRegistry;

class PresetStore {
public:
    // Load a preset JSON file and build the EffectChain via the registry.
    // Returns nullopt on parse error or missing required fields.
    static std::optional<Preset> loadFromFile(
        const std::string&   path,
        EffectChain&         chain,
        EffectNodeRegistry&  registry,
        double               sampleRate,
        int                  maxBlockSize
    );

    // Save the current chain state back to a JSON file.
    static bool saveToFile(
        const std::string& path,
        const Preset&      preset,
        const EffectChain& chain
    );

    // Parse raw JSON string (useful for unit tests / BLE transfer).
    static std::optional<Preset> loadFromJson(
        const nlohmann::json& j,
        EffectChain&          chain,
        EffectNodeRegistry&   registry,
        double                sampleRate,
        int                   maxBlockSize
    );
};

} // namespace gearboxfx
