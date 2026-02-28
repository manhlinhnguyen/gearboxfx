#include "PresetPanel.h"
#include <imgui.h>
#include <filesystem>
#include <algorithm>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace gearboxfx {

void PresetPanel::scanPresets(const std::string& dir) {
    m_presets.clear();
    std::error_code ec;
    if (!fs::exists(dir, ec)) return;

    for (auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.path().extension() == ".json") {
            PresetEntry pe;
            pe.path = entry.path().string();
            pe.name = entry.path().stem().string();
            m_presets.push_back(std::move(pe));
        }
    }
    std::sort(m_presets.begin(), m_presets.end(),
              [](const PresetEntry& a, const PresetEntry& b) {
                  return a.name < b.name;
              });
}

void PresetPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(400, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(280, 155), ImGuiCond_Always);
    ImGui::Begin("Presets", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

    // Refresh button
    if (ImGui::Button("Refresh") || m_needScan) {
        scanPresets(ctx.presetsDir);
        m_needScan = false;
    }

    ImGui::Separator();

    // Scrollable preset list
    ImGui::BeginChild("preset_list", ImVec2(0, -40.0f), true);
    for (int i = 0; i < static_cast<int>(m_presets.size()); ++i) {
        bool selected = (m_selectedIdx == i);
        if (ImGui::Selectable(m_presets[i].name.c_str(), selected)) {
            m_selectedIdx = i;
            // Hold chain lock so the audio callback cannot iterate the chain
            // while loadPreset() tears it down and rebuilds it.
            auto lock = ctx.audio->lockChain();
            if (ctx.engine->loadPreset(m_presets[i].path)) {
                ctx.engine->chain().prepare(ctx.sampleRate, ctx.blockSize);
                ctx.selectedEffectId->clear();
            }
        }
    }
    ImGui::EndChild();

    ImGui::Separator();

    // New preset
    if (ImGui::Button("+ New")) {
        auto lock = ctx.audio->lockChain();
        ctx.engine->chain().clear();
        ctx.selectedEffectId->clear();
        ctx.engine->setPresetName("new_preset");
        m_selectedIdx = -1;
    }

    ImGui::SameLine();

    // Save As
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 70.0f);
    ImGui::InputText("##savename", m_saveName, sizeof(m_saveName));
    ImGui::SameLine();
    if (ImGui::Button("Save")) {
        std::string path = ctx.presetsDir + "/" + m_saveName + ".json";
        ctx.engine->setPresetName(m_saveName);
        if (ctx.engine->savePreset(path)) {
            spdlog::info("PresetPanel: saved '{}'", path);
            m_needScan = true;
        }
    }

    ImGui::End();
}

} // namespace gearboxfx
