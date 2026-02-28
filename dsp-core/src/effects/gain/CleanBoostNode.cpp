#include "effects/gain/CleanBoostNode.h"
#include <cmath>

namespace gearboxfx {

CleanBoostNode::CleanBoostNode() {
    registerParam("gain_db", {0.0f, -20.0f, 20.0f, "Gain", "dB"});
}

void CleanBoostNode::onParamChanged(const std::string& name, float /*value*/) {
    if (name == "gain_db")
        m_gainLin = std::pow(10.0f, getParam("gain_db") / 20.0f);
}

void CleanBoostNode::process(AudioBufferView input, AudioBufferView output, int numSamples) {
    for (int c = 0; c < output.numChannels; ++c)
        for (int s = 0; s < numSamples; ++s)
            output[c][s] = input[c][s] * m_gainLin;
}

} // namespace gearboxfx
