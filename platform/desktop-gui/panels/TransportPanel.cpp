#include "TransportPanel.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace gearboxfx {

void TransportPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(400, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Transport");

    // ── WAV file path ────────────────────────────────────────────────────────
    ImGui::Text("WAV file:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
    ImGui::InputText("##wav", m_wavPath, sizeof(m_wavPath));
    ImGui::SameLine();

    if (ImGui::Button("Load")) {
        if (ctx.audio->loadFile(m_wavPath)) {
            ctx.sampleRate = static_cast<double>(ctx.audio->sampleRate());
            ctx.audio->setEngine(ctx.engine, ctx.sampleRate, ctx.blockSize);
            ctx.engine->prepare(ctx.sampleRate, ctx.blockSize);
        }
    }

    ImGui::Separator();

    // ── Transport controls ───────────────────────────────────────────────────
    bool playing = ctx.audio->isPlaying();

    if (ImGui::Button(playing ? " Stop " : " Play ")) {
        if (playing)
            ctx.audio->stop();
        else
            ctx.audio->play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Rewind"))
        ctx.audio->seekToStart();

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

    ImVec2 vuPos  = ImGui::GetCursorScreenPos();
    float  vuW    = ImGui::GetContentRegionAvail().x;
    float  vuH    = 16.0f;
    ImVec2 vuMax  = ImVec2(vuPos.x + vuW, vuPos.y + vuH);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(vuPos, vuMax, IM_COL32(30, 30, 30, 255), 2.0f);

    float fill = std::min(level, 1.0f) * vuW;
    ImU32 col = (level < 0.7f)  ? IM_COL32(40, 200, 40, 255)
              : (level < 0.95f) ? IM_COL32(220, 200, 0, 255)
                                : IM_COL32(220, 50, 50, 255);
    if (fill > 0.0f)
        dl->AddRectFilled(vuPos, ImVec2(vuPos.x + fill, vuMax.y), col, 2.0f);

    // Tick marks at -6, -3, 0 dB (0.5, 0.71, 1.0 amplitude)
    const float ticks[] = {0.5f, 0.71f, 1.0f};
    for (float t : ticks) {
        float tx = vuPos.x + t * vuW;
        dl->AddLine(ImVec2(tx, vuPos.y), ImVec2(tx, vuMax.y),
                    IM_COL32(180, 180, 180, 120), 1.0f);
    }

    ImGui::Dummy(ImVec2(vuW, vuH + 2.0f));
    ImGui::Text("Level: %.3f", level);

    ImGui::End();
}

} // namespace gearboxfx
