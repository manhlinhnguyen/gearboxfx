#include "GearBoxApp.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace gearboxfx {

GearBoxApp::GearBoxApp()  = default;
GearBoxApp::~GearBoxApp() = default;

bool GearBoxApp::init(int width, int height, const char* title,
                      const std::string& presetsDir)
{
    m_presetsDir = presetsDir;

    if (!glfwInit()) {
        spdlog::error("GearBoxApp: glfwInit failed");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        spdlog::error("GearBoxApp: glfwCreateWindow failed");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = "gearboxfx_gui.ini";  // persist window layout
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    spdlog::info("GearBoxApp: initialized ({}x{}, presets='{}')",
                 width, height, presetsDir);
    return true;
}

void GearBoxApp::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // render() creates AppContext from m_* members, renders all panels,
        // then syncs changed values (e.g. sampleRate after WAV load) back to m_*.
        render();

        ImGui::Render();

        int fbW, fbH;
        glfwGetFramebufferSize(m_window, &fbW, &fbH);
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void GearBoxApp::render() {
    AppContext ctx;
    ctx.engine           = &m_engine;
    ctx.audio            = &m_audio;
    ctx.selectedEffectId = &m_selectedEffectId;
    ctx.presetsDir       = m_presetsDir;
    ctx.sampleRate       = m_sampleRate;
    ctx.blockSize        = m_blockSize;

    m_transport.render(ctx);
    m_presets.render(ctx);
    m_chain.render(ctx);
    m_params.render(ctx);

    // Sync back mutable fields that panels may have changed
    m_sampleRate = ctx.sampleRate;
}

void GearBoxApp::shutdown() {
    m_audio.stop();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();

    spdlog::info("GearBoxApp: shutdown complete");
}

} // namespace gearboxfx
