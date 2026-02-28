#include "ParameterManager.h"
#include "EffectChain.h"
#include <stdexcept>

namespace gearboxfx {

std::pair<std::string, std::string> ParameterManager::parseKey(const std::string& key) {
    auto dot = key.find('.');
    if (dot == std::string::npos)
        throw std::runtime_error("ParameterManager: invalid key format (expected 'id.param'): " + key);
    // Find the second dot to handle category.name.param keys
    auto dot2 = key.find('.', dot + 1);
    if (dot2 != std::string::npos) {
        // Key is "effect_id.param_name" where effect_id can contain dots?
        // We treat everything before the LAST dot as effect_id.
        std::string effectId = key.substr(0, dot2);
        std::string paramName = key.substr(dot2 + 1);
        return {effectId, paramName};
    }
    return {key.substr(0, dot), key.substr(dot + 1)};
}

bool ParameterManager::set(const std::string& key, float value) {
    if (!m_chain) return false;

    auto [effectId, paramName] = parseKey(key);

    auto node = m_chain->findNode(effectId);
    if (!node) return false;

    if (!node->setParam(paramName, value)) return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[key] = value;
    return true;
}

std::optional<float> ParameterManager::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(key);
    if (it == m_cache.end()) return std::nullopt;
    return it->second;
}

void ParameterManager::syncFromChain() {
    if (!m_chain) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache.clear();

    for (auto& node : m_chain->nodes()) {
        for (auto& [paramName, val] : node->paramValues()) {
            std::string key = node->id() + "." + paramName;
            m_cache[key] = val;
        }
    }
}

} // namespace gearboxfx
