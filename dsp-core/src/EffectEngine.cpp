#include "EffectEngine.h"
#include <spdlog/spdlog.h>
#include <cstring>

namespace gearboxfx {

EffectEngine::EffectEngine() {
    m_paramManager.attachChain(&m_chain);
}

void EffectEngine::prepare(double sampleRate, int maxBlockSize) {
    m_sampleRate   = sampleRate;
    m_maxBlockSize = maxBlockSize;
    m_chain.prepare(sampleRate, maxBlockSize);
    m_prepared = true;
    spdlog::info("EffectEngine prepared: {}Hz, block={}", (int)sampleRate, maxBlockSize);
}

void EffectEngine::processBlock(AudioBufferView input, AudioBufferView output, int numSamples) {
    if (m_bypass) {
        // Pass-through
        for (int c = 0; c < output.numChannels; ++c)
            std::memcpy(output[c], input[c], numSamples * sizeof(float));
        return;
    }

    // Apply output volume after chain (simple scalar)
    m_chain.process(input, output, numSamples);

    float vol = m_currentPreset.outputVolume;
    if (vol != 1.0f) {
        for (int c = 0; c < output.numChannels; ++c)
            for (int s = 0; s < numSamples; ++s)
                output[c][s] *= vol;
    }
}

bool EffectEngine::loadPreset(const std::string& path) {
    auto result = PresetStore::loadFromFile(
        path, m_chain, m_registry, m_sampleRate, m_maxBlockSize);
    if (!result) {
        spdlog::error("EffectEngine: failed to load preset '{}'", path);
        return false;
    }
    m_currentPreset = *result;
    m_paramManager.syncFromChain();
    spdlog::info("EffectEngine: loaded preset '{}' from '{}'", m_currentPreset.name, path);
    return true;
}

bool EffectEngine::savePreset(const std::string& path) const {
    return PresetStore::saveToFile(path, m_currentPreset, m_chain);
}

bool EffectEngine::setParam(const std::string& effectId_paramName, float value) {
    return m_paramManager.set(effectId_paramName, value);
}

} // namespace gearboxfx
