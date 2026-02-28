#include "PresetStore.h"
#include "effects/EffectNodeRegistry.h"
#include <fstream>
#include <spdlog/spdlog.h>

namespace gearboxfx {

static std::optional<Preset> buildFromJson(
    const nlohmann::json& j,
    EffectChain&          chain,
    EffectNodeRegistry&   registry,
    double                sampleRate,
    int                   maxBlockSize)
{
    try {
        Preset p;
        p.raw           = j;
        p.presetId      = j.value("preset_id", "");
        p.formatVersion = j.value("format_version", "1.0");
        p.name          = j.value("name", "Unnamed");
        p.routingMode   = j.value("routing_mode", "serial");

        if (j.contains("output_eq")) {
            auto& eq = j["output_eq"];
            p.bassDb   = eq.value("bass_db",   0.0f);
            p.midDb    = eq.value("mid_db",    0.0f);
            p.trebleDb = eq.value("treble_db", 0.0f);
        }
        p.outputVolume = j.value("output_volume", 0.85f);

        chain.clear();

        if (!j.contains("effect_chain") || !j["effect_chain"].is_array()) {
            spdlog::warn("Preset '{}': no effect_chain array", p.name);
            chain.prepare(sampleRate, maxBlockSize);
            return p;
        }

        for (auto& nodeJson : j["effect_chain"]) {
            std::string typeId  = nodeJson.value("type", "");
            std::string nodeId  = nodeJson.value("id",   "");
            bool        enabled = nodeJson.value("enabled", true);

            auto node = registry.create(typeId);
            if (!node) {
                spdlog::error("Preset '{}': unknown effect type '{}'", p.name, typeId);
                continue;
            }

            node->setId(nodeId);
            node->setEnabled(enabled);

            if (nodeJson.contains("params") && nodeJson["params"].is_object())
                node->loadParams(nodeJson["params"]);

            chain.addNode(node);
        }

        chain.prepare(sampleRate, maxBlockSize);
        return p;

    } catch (const nlohmann::json::exception& e) {
        spdlog::error("PresetStore JSON error: {}", e.what());
        return std::nullopt;
    }
}

std::optional<Preset> PresetStore::loadFromFile(
    const std::string&   path,
    EffectChain&         chain,
    EffectNodeRegistry&  registry,
    double               sampleRate,
    int                  maxBlockSize)
{
    std::ifstream f(path);
    if (!f.is_open()) {
        spdlog::error("PresetStore: cannot open '{}'", path);
        return std::nullopt;
    }

    nlohmann::json j;
    try {
        f >> j;
    } catch (const nlohmann::json::parse_error& e) {
        spdlog::error("PresetStore: JSON parse error in '{}': {}", path, e.what());
        return std::nullopt;
    }

    return buildFromJson(j, chain, registry, sampleRate, maxBlockSize);
}

std::optional<Preset> PresetStore::loadFromJson(
    const nlohmann::json& j,
    EffectChain&          chain,
    EffectNodeRegistry&   registry,
    double                sampleRate,
    int                   maxBlockSize)
{
    return buildFromJson(j, chain, registry, sampleRate, maxBlockSize);
}

bool PresetStore::saveToFile(
    const std::string& path,
    const Preset&      preset,
    const EffectChain& chain)
{
    nlohmann::json j;
    j["preset_id"]      = preset.presetId;
    j["format_version"] = preset.formatVersion;
    j["name"]           = preset.name;
    j["routing_mode"]   = preset.routingMode;
    j["output_eq"]      = {
        {"bass_db",   preset.bassDb},
        {"mid_db",    preset.midDb},
        {"treble_db", preset.trebleDb}
    };
    j["output_volume"]  = preset.outputVolume;

    nlohmann::json chainArr = nlohmann::json::array();
    for (auto& node : chain.nodes()) {
        nlohmann::json nodeJson;
        nodeJson["id"]      = node->id();
        nodeJson["type"]    = node->typeId();
        nodeJson["enabled"] = node->isEnabled();
        nodeJson["params"]  = node->saveParams();
        chainArr.push_back(nodeJson);
    }
    j["effect_chain"] = chainArr;

    std::ofstream f(path);
    if (!f.is_open()) {
        spdlog::error("PresetStore: cannot write '{}'", path);
        return false;
    }

    f << j.dump(2);
    return f.good();
}

} // namespace gearboxfx
