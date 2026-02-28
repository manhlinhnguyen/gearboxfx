#pragma once
#include "EffectChain.h"
#include "ParameterManager.h"
#include "PresetStore.h"
#include "AudioBuffer.h"
#include "effects/EffectNodeRegistry.h"
#include <string>
#include <memory>

namespace gearboxfx {

// Top-level orchestrator. Owns the EffectChain, ParameterManager, and EffectNodeRegistry.
class EffectEngine {
public:
    EffectEngine();

    // Must be called before processBlock().
    void prepare(double sampleRate, int maxBlockSize);

    // Process one block of audio. Input and output must have the same layout.
    void processBlock(AudioBufferView input, AudioBufferView output, int numSamples);

    // Load a preset from a JSON file. Rebuilds the chain.
    bool loadPreset(const std::string& path);

    // Save current chain state to a JSON file.
    bool savePreset(const std::string& path) const;

    // Bypass the entire effect chain (pass audio through unchanged).
    void setBypass(bool bypass) { m_bypass = bypass; }
    bool isBypassed()   const  { return m_bypass; }

    // Output volume scalar applied after the chain (thread-safe — GUI thread only).
    void  setOutputVolume(float v) { m_currentPreset.outputVolume = v; }
    float outputVolume()    const  { return m_currentPreset.outputVolume; }

    // Update the in-memory preset name (used before savePreset).
    void               setPresetName(const std::string& n) { m_currentPreset.name = n; }
    const std::string& presetName()                  const { return m_currentPreset.name; }

    // Thread-safe parameter update (can be called from any thread).
    bool setParam(const std::string& effectId_paramName, float value);

    EffectChain&      chain()            { return m_chain; }
    ParameterManager& parameterManager() { return m_paramManager; }
    EffectNodeRegistry& registry()       { return m_registry; }

    const Preset& currentPreset() const  { return m_currentPreset; }

private:
    EffectChain        m_chain;
    ParameterManager   m_paramManager;
    EffectNodeRegistry m_registry;
    Preset             m_currentPreset;

    double m_sampleRate   = 48000.0;
    int    m_maxBlockSize = 256;
    bool   m_bypass       = false;
    bool   m_prepared     = false;
};

} // namespace gearboxfx
