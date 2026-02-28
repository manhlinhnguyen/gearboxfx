#pragma once
#include "EffectEngine.h"
#include "GuiAudioIO.h"
#include <string>

namespace gearboxfx {

// Shared application state passed by pointer to every panel each frame.
struct AppContext {
    EffectEngine* engine           = nullptr;
    GuiAudioIO*   audio            = nullptr;
    std::string*  selectedEffectId = nullptr;  // ID of effect selected in ChainPanel
    std::string   presetsDir;
    double        sampleRate = 48000.0;
    int           blockSize  = 256;
};

} // namespace gearboxfx
