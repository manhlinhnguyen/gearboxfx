#pragma once
#include "AudioBuffer.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>

namespace gearboxfx {

struct ParamDef {
    float defaultValue;
    float minValue;
    float maxValue;
    std::string label;   // human-readable name
    std::string unit;    // "dB", "ms", "Hz", ""
};

class EffectNode {
public:
    virtual ~EffectNode() = default;

    // Called once when the effect is inserted into a chain.
    void prepare(double sampleRate, int maxBlockSize) {
        m_sampleRate   = sampleRate;
        m_maxBlockSize = maxBlockSize;
        onPrepare(sampleRate, maxBlockSize);
    }

    // Called once when removed from chain or engine is shut down.
    virtual void reset() {}

    // Process audio. Input and output may be the same buffer (in-place).
    virtual void process(AudioBufferView input, AudioBufferView output, int numSamples) = 0;

    // ── Parameter API ────────────────────────────────────────────────────────

    void registerParam(const std::string& name, const ParamDef& def) {
        m_paramDefs[name]   = def;
        m_paramValues[name] = def.defaultValue;
    }

    // Returns false if name not found or value out of range (clamped).
    bool setParam(const std::string& name, float value) {
        auto it = m_paramDefs.find(name);
        if (it == m_paramDefs.end()) return false;
        value = std::max(it->second.minValue, std::min(it->second.maxValue, value));
        m_paramValues[name] = value;
        onParamChanged(name, value);
        return true;
    }

    float getParam(const std::string& name) const {
        auto it = m_paramValues.find(name);
        if (it == m_paramValues.end()) throw std::runtime_error("Unknown param: " + name);
        return it->second;
    }

    bool hasParam(const std::string& name) const {
        return m_paramDefs.count(name) > 0;
    }

    const std::unordered_map<std::string, ParamDef>&   paramDefs()   const { return m_paramDefs; }
    const std::unordered_map<std::string, float>&       paramValues() const { return m_paramValues; }

    // ── JSON round-trip ──────────────────────────────────────────────────────

    void loadParams(const nlohmann::json& j) {
        for (auto& [name, val] : j.items())
            setParam(name, val.get<float>());
    }

    nlohmann::json saveParams() const {
        nlohmann::json j;
        for (auto& [name, val] : m_paramValues)
            j[name] = val;
        return j;
    }

    // ── Identity ─────────────────────────────────────────────────────────────

    void setId(const std::string& id)     { m_id = id; }
    const std::string& id()     const     { return m_id; }

    void setTypeId(const std::string& tid) { m_typeId = tid; }
    const std::string& typeId() const      { return m_typeId; }

    bool isEnabled() const                 { return m_enabled; }
    void setEnabled(bool en)               { m_enabled = en; }

protected:
    virtual void onPrepare(double /*sampleRate*/, int /*maxBlockSize*/) {}
    virtual void onParamChanged(const std::string& /*name*/, float /*value*/) {}

    double m_sampleRate   = 48000.0;
    int    m_maxBlockSize = 256;

private:
    std::string  m_id;
    std::string  m_typeId;
    bool         m_enabled = true;
    std::unordered_map<std::string, ParamDef> m_paramDefs;
    std::unordered_map<std::string, float>    m_paramValues;
};

} // namespace gearboxfx
