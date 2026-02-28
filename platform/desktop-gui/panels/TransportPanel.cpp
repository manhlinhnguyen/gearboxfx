#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "TransportPanel.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace gearboxfx {

static std::string openWavDialog() {
    char buf[512] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Audio Files\0*.wav;*.mp3;*.ogg\0WAV Files\0*.wav\0MP3 Files\0*.mp3\0OGG Files\0*.ogg\0All Files\0*.*\0";
    ofn.lpstrFile   = buf;
    ofn.nMaxFile    = sizeof(buf);
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn))
        return buf;
    return {};
}

void TransportPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(400, 155), ImGuiCond_Always);
    ImGui::Begin("Transport", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // ── File label + Browse ──────────────────────────────────────────────────
    const char* fname = m_loadedFile[0] ? m_loadedFile : "(none)";
    // Truncate display to fit — show just the filename portion
    const char* lastSlash = std::strrchr(fname, '\\');
    if (!lastSlash) lastSlash = std::strrchr(fname, '/');
    const char* displayName = lastSlash ? lastSlash + 1 : fname;

    ImGui::Text("File: %s", displayName);
    ImGui::SameLine();
    if (ImGui::Button("Browse...")) {
        std::string path = openWavDialog();
        if (!path.empty()) {
            std::strncpy(m_loadedFile, path.c_str(), sizeof(m_loadedFile) - 1);
            m_loadedFile[sizeof(m_loadedFile) - 1] = '\0';
            if (ctx.audio->loadFile(m_loadedFile)) {
                ctx.sampleRate = static_cast<double>(ctx.audio->sampleRate());
                ctx.audio->setEngine(ctx.engine, ctx.sampleRate, ctx.blockSize);
                ctx.engine->prepare(ctx.sampleRate, ctx.blockSize);
            }
        }
    }

    ImGui::Separator();

    // ── Transport controls ───────────────────────────────────────────────────
    bool playing = ctx.audio->isPlaying();
    if (ImGui::Button(playing ? " Stop " : " Play ")) {
        if (playing) ctx.audio->stop();
        else         ctx.audio->play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rewind"))
        ctx.audio->seekToStart();
    ImGui::SameLine();
    bool looping = ctx.audio->loop();
    if (ImGui::Checkbox("Loop", &looping))
        ctx.audio->setLoop(looping);

    // Progress bar
    float prog = ctx.audio->progress();
    ImGui::ProgressBar(prog, ImVec2(-1.0f, 0.0f));

    ImGui::Separator();

    // ── Output volume ────────────────────────────────────────────────────────
    float vol = ctx.engine->outputVolume();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::SliderFloat("##vol", &vol, 0.0f, 1.5f, "Vol: %.2f"))
        ctx.engine->setOutputVolume(vol);

    // ── VU meter ─────────────────────────────────────────────────────────────
    float level = ctx.audio->outputLevel();
    ImVec2 vuPos = ImGui::GetCursorScreenPos();
    float  vuW   = ImGui::GetContentRegionAvail().x;
    float  vuH   = 12.0f;
    ImVec2 vuMax = ImVec2(vuPos.x + vuW, vuPos.y + vuH);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(vuPos, vuMax, IM_COL32(30, 30, 30, 255), 2.0f);

    float fill = std::min(level, 1.0f) * vuW;
    ImU32 col = (level < 0.7f)  ? IM_COL32(40, 200, 40, 255)
              : (level < 0.95f) ? IM_COL32(220, 200, 0, 255)
                                : IM_COL32(220, 50, 50, 255);
    if (fill > 0.0f)
        dl->AddRectFilled(vuPos, ImVec2(vuPos.x + fill, vuMax.y), col, 2.0f);

    const float ticks[] = {0.5f, 0.71f, 1.0f};
    for (float t : ticks) {
        float tx = vuPos.x + t * vuW;
        dl->AddLine(ImVec2(tx, vuPos.y), ImVec2(tx, vuMax.y),
                    IM_COL32(180, 180, 180, 120), 1.0f);
    }
    ImGui::Dummy(ImVec2(vuW, vuH + 2.0f));

    ImGui::End();
}

} // namespace gearboxfx
