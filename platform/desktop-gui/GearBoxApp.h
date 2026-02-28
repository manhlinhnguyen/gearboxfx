#pragma once
#include "AppContext.h"
#include "EffectEngine.h"
#include "GuiAudioIO.h"
#include "panels/TransportPanel.h"
#include "panels/PresetPanel.h"
#include "panels/ChainPanel.h"
#include "panels/ParamPanel.h"
#include <string>

struct GLFWwindow;

namespace gearboxfx {

class GearBoxApp {
public:
    GearBoxApp();
    ~GearBoxApp();

    // Create GLFW window + ImGui context. Returns false on failure.
    bool init(int width, int height, const char* title, const std::string& presetsDir = "presets");

    // Enter the render loop (blocks until window is closed).
    void run();

    // Tear down ImGui + GLFW.
    void shutdown();

private:
    void render();

    GLFWwindow* m_window = nullptr;

    EffectEngine   m_engine;
    GuiAudioIO     m_audio;
    std::string    m_selectedEffectId;
    std::string    m_presetsDir;
    double         m_sampleRate = 48000.0;
    int            m_blockSize  = 256;

    TransportPanel m_transport;
    PresetPanel    m_presets;
    ChainPanel     m_chain;
    ParamPanel     m_params;
};

} // namespace gearboxfx
