#include "ParamPanel.h"
#include "EffectChain.h"
#include "EffectNode.h"
#include <imgui.h>
#include <imgui-knobs.h>
#include <algorithm>
#include <string>
#include <vector>

namespace gearboxfx {

void ParamPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(0, 200), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(1060, 340), ImGuiCond_FirstUseEver);
    ImGui::Begin("Parameters");

    if (ctx.selectedEffectId->empty()) {
        ImGui::TextDisabled("Select an effect in the chain to edit its parameters.");
        ImGui::End();
        return;
    }

    auto node = ctx.engine->chain().findNode(*ctx.selectedEffectId);
    if (!node) {
        ImGui::TextDisabled("Effect '%s' not found.", ctx.selectedEffectId->c_str());
        ImGui::End();
        return;
    }

    ImGui::Text("%s", node->id().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("— %s", node->typeId().c_str());
    ImGui::Separator();

    const auto& defs   = node->paramDefs();
    const auto& values = node->paramValues();

    // Sort parameter names for a stable, deterministic layout
    std::vector<std::string> names;
    names.reserve(defs.size());
    for (const auto& [name, _] : defs)
        names.push_back(name);
    std::sort(names.begin(), names.end());

    // Render knobs (normalized/small range) side-by-side
    bool firstKnob = true;
    for (const auto& name : names) {
        const auto& def = defs.at(name);
        float range = def.maxValue - def.minValue;
        if (range > 2.0f) continue;  // handled below as slider

        float v = values.at(name);
        ImGui::PushID(name.c_str());
        if (!firstKnob) ImGui::SameLine();

        if (ImGuiKnobs::Knob(def.label.c_str(), &v,
                              def.minValue, def.maxValue,
                              0.0f, "%.2f",
                              ImGuiKnobVariant_Wiper)) {
            ctx.engine->setParam(node->id() + "." + name, v);
        }
        if (ImGui::IsItemHovered() && !def.unit.empty())
            ImGui::SetTooltip("%s (%s)", def.label.c_str(), def.unit.c_str());

        firstKnob = false;
        ImGui::PopID();
    }

    if (!firstKnob) ImGui::NewLine();  // end knob row

    // Render sliders (wider ranges: time, gain_db, bpm, etc.)
    for (const auto& name : names) {
        const auto& def = defs.at(name);
        float range = def.maxValue - def.minValue;
        if (range <= 2.0f) continue;  // already rendered as knob

        float v = values.at(name);
        ImGui::PushID(name.c_str());

        std::string label = def.label;
        if (!def.unit.empty())
            label += " (" + def.unit + ")";

        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::SliderFloat(label.c_str(), &v, def.minValue, def.maxValue)) {
            ctx.engine->setParam(node->id() + "." + name, v);
        }

        ImGui::PopID();
    }

    ImGui::End();
}

} // namespace gearboxfx
