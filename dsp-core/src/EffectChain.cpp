#include "EffectChain.h"
#include <algorithm>
#include <cstring>

namespace gearboxfx {

void EffectChain::prepare(double sampleRate, int maxBlockSize) {
    m_sampleRate   = sampleRate;
    m_maxBlockSize = maxBlockSize;

    m_pingBuf.resize(2, maxBlockSize);
    m_pongBuf.resize(2, maxBlockSize);

    for (auto& node : m_nodes)
        node->prepare(sampleRate, maxBlockSize);
}

void EffectChain::addNode(std::shared_ptr<EffectNode> node) {
    node->prepare(m_sampleRate, m_maxBlockSize);
    m_nodes.push_back(std::move(node));
}

void EffectChain::insertNode(int index, std::shared_ptr<EffectNode> node) {
    node->prepare(m_sampleRate, m_maxBlockSize);
    index = std::max(0, std::min(index, (int)m_nodes.size()));
    m_nodes.insert(m_nodes.begin() + index, std::move(node));
}

bool EffectChain::removeNode(const std::string& effectId) {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&](const auto& n){ return n->id() == effectId; });
    if (it == m_nodes.end()) return false;
    (*it)->reset();
    m_nodes.erase(it);
    return true;
}

bool EffectChain::replaceNode(const std::string& effectId, std::shared_ptr<EffectNode> newNode) {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&](const auto& n){ return n->id() == effectId; });
    if (it == m_nodes.end()) return false;
    (*it)->reset();
    newNode->prepare(m_sampleRate, m_maxBlockSize);
    *it = std::move(newNode);
    return true;
}

std::shared_ptr<EffectNode> EffectChain::findNode(const std::string& effectId) const {
    auto it = std::find_if(m_nodes.begin(), m_nodes.end(),
        [&](const auto& n){ return n->id() == effectId; });
    return (it != m_nodes.end()) ? *it : nullptr;
}

void EffectChain::clear() {
    for (auto& n : m_nodes) n->reset();
    m_nodes.clear();
}

void EffectChain::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    if (m_nodes.empty()) {
        // Pass-through
        for (int c = 0; c < output.numChannels; ++c)
            std::memcpy(output[c], input[c], numSamples * sizeof(float));
        return;
    }

    const int numCh = input.numChannels;

    // Set up ping-pong: ping receives first node output, pong receives second, etc.
    // src always points to the current read buffer, dst to the write buffer.
    AudioBuffer* src = &m_pingBuf;
    AudioBuffer* dst = &m_pongBuf;

    // Copy input into ping buffer.
    for (int c = 0; c < numCh; ++c)
        std::memcpy(src->getWritePointer(c), input[c], numSamples * sizeof(float));

    for (size_t i = 0; i < m_nodes.size(); ++i) {
        auto& node = m_nodes[i];

        AudioBufferView srcView = src->view();
        srcView.numChannels = numCh;
        srcView.numSamples  = numSamples;

        AudioBufferView dstView = dst->view();
        dstView.numChannels = numCh;
        dstView.numSamples  = numSamples;

        if (node->isEnabled()) {
            node->process(srcView, dstView, numSamples);
        } else {
            // Bypass: just copy
            for (int c = 0; c < numCh; ++c)
                std::memcpy(dstView[c], srcView[c], numSamples * sizeof(float));
        }

        std::swap(src, dst);
    }

    // After the last node, src points to the buffer with the final result.
    for (int c = 0; c < numCh; ++c)
        std::memcpy(output[c], src->getWritePointer(c), numSamples * sizeof(float));
}

} // namespace gearboxfx
