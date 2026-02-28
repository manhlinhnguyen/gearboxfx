#pragma once
#include "../EffectNode.h"
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>

namespace gearboxfx {

// Singleton factory for EffectNode subclasses.
// Register a new type with: registry.reg<MyNode>("category.my_name");
class EffectNodeRegistry {
public:
    using FactoryFn = std::function<std::shared_ptr<EffectNode>()>;

    EffectNodeRegistry() { registerAll(); }

    template<typename T>
    void reg(const std::string& typeId) {
        m_factories[typeId] = [typeId]() {
            auto node = std::make_shared<T>();
            node->setTypeId(typeId);
            return node;
        };
    }

    // Create a node by typeId. Returns nullptr if not found.
    std::shared_ptr<EffectNode> create(const std::string& typeId) const {
        auto it = m_factories.find(typeId);
        if (it == m_factories.end()) return nullptr;
        return it->second();
    }

    bool has(const std::string& typeId) const {
        return m_factories.count(typeId) > 0;
    }

    std::vector<std::string> registeredTypes() const {
        std::vector<std::string> out;
        out.reserve(m_factories.size());
        for (auto& [k, _] : m_factories) out.push_back(k);
        return out;
    }

private:
    // Registers all built-in effect types.
    void registerAll();

    std::unordered_map<std::string, FactoryFn> m_factories;
};

} // namespace gearboxfx
