#pragma once
#include "EffectNode.h"
#include "AudioBuffer.h"
#include <vector>
#include <memory>
#include <string>

namespace gearboxfx {

// Ordered chain of EffectNodes.
// Uses a ping-pong pair of AudioBuffers so each node reads from one and writes to the other,
// avoiding in-place aliasing and simplifying node implementations.
class EffectChain {
public:
    void prepare(double sampleRate, int maxBlockSize);

    // Append a node to the end of the chain.
    void addNode(std::shared_ptr<EffectNode> node);

    // Insert at position (0 = before first node).
    void insertNode(int index, std::shared_ptr<EffectNode> node);

    // Remove by effect instance id.
    bool removeNode(const std::string& effectId);

    // Replace one effect with another (preserving position).
    bool replaceNode(const std::string& effectId, std::shared_ptr<EffectNode> newNode);

    std::shared_ptr<EffectNode> findNode(const std::string& effectId) const;

    const std::vector<std::shared_ptr<EffectNode>>& nodes() const { return m_nodes; }

    void clear();

    // Process the full chain: input → [node0] → [node1] → ... → output.
    // Enabled nodes are processed; disabled nodes pass audio through.
    // numSamples must be ≤ maxBlockSize passed to prepare().
    void process(AudioBufferView input, AudioBufferView output, int numSamples);

private:
    std::vector<std::shared_ptr<EffectNode>> m_nodes;

    // Ping-pong buffers (owned by chain, not by nodes)
    AudioBuffer m_pingBuf;
    AudioBuffer m_pongBuf;

    double m_sampleRate   = 48000.0;
    int    m_maxBlockSize = 256;
};

} // namespace gearboxfx
