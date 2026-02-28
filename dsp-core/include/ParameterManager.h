#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <optional>

namespace gearboxfx {

class EffectChain;

// Thread-safe parameter store.
// Keys use the format "effect_id.param_name".
// Updates are forwarded to the EffectNode in the chain.
class ParameterManager {
public:
    // Attach to a chain so set() can propagate changes to nodes.
    void attachChain(EffectChain* chain) { m_chain = chain; }

    // Set a parameter. Thread-safe (acquires mutex).
    // Returns false if effect or param not found.
    bool set(const std::string& key, float value);

    // Get current value. Returns nullopt if not found.
    std::optional<float> get(const std::string& key) const;

    // Rebuild local cache from the current chain state (call after loadPreset).
    void syncFromChain();

private:
    static std::pair<std::string, std::string> parseKey(const std::string& key);

    EffectChain*                            m_chain = nullptr;
    mutable std::mutex                      m_mutex;
    std::unordered_map<std::string, float>  m_cache;
};

} // namespace gearboxfx
