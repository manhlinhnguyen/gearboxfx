#include "ChainPanel.h"
#include "EffectChain.h"
#include "effects/EffectNodeRegistry.h"
#include <imgui.h>
#include <algorithm>
#include <string>

namespace gearboxfx {

void ChainPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(680, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Effect Chain");

    const auto& nodes = ctx.engine->chain().nodes();

    // Iterate a copy of indices so we can break after mutation
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        auto node = nodes[i];  // shared_ptr copy before any mutation
        ImGui::PushID(i);

        // Enable/disable checkbox
        bool en = node->isEnabled();
        if (ImGui::Checkbox("##en", &en))
            node->setEnabled(en);

        ImGui::SameLine();

        // Effect instance ID (selectable)
        bool selected = (*ctx.selectedEffectId == node->id());
        if (ImGui::Selectable(node->id().c_str(), selected,
                              ImGuiSelectableFlags_None, ImVec2(120.0f, 0.0f)))
            *ctx.selectedEffectId = node->id();

        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", node->typeId().c_str());
        ImGui::SameLine();

        // Move up
        if (ImGui::SmallButton("^")) {
            if (i > 0) {
                auto lock = ctx.audio->lockChain();
                ctx.engine->chain().removeNode(node->id());
                ctx.engine->chain().insertNode(i - 1, node);
            }
            ImGui::PopID();
            break;  // nodes vector is now stale
        }
        ImGui::SameLine();

        // Move down
        if (ImGui::SmallButton("v")) {
            if (i < static_cast<int>(nodes.size()) - 1) {
                auto lock = ctx.audio->lockChain();
                ctx.engine->chain().removeNode(node->id());
                ctx.engine->chain().insertNode(i + 1, node);
            }
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();

        // Remove
        if (ImGui::SmallButton("X")) {
            auto lock = ctx.audio->lockChain();
            if (*ctx.selectedEffectId == node->id())
                ctx.selectedEffectId->clear();
            ctx.engine->chain().removeNode(node->id());
            ImGui::PopID();
            break;
        }

        ImGui::PopID();
    }

    ImGui::Separator();

    // ── Add Effect popup ─────────────────────────────────────────────────────
    if (ImGui::Button("+ Add Effect"))
        ImGui::OpenPopup("add_effect_popup");

    if (ImGui::BeginPopup("add_effect_popup")) {
        auto types = ctx.engine->registry().registeredTypes();
        std::sort(types.begin(), types.end());

        for (const auto& typeId : types) {
            if (ImGui::MenuItem(typeId.c_str())) {
                auto node = ctx.engine->registry().create(typeId);
                if (node) {
                    // Build unique ID: replace dots with underscores + counter
                    std::string id = typeId;
                    std::replace(id.begin(), id.end(), '.', '_');
                    id += "_" + std::to_string(m_addCounter++);
                    node->setId(id);
                    node->prepare(ctx.sampleRate, ctx.blockSize);

                    auto lock = ctx.audio->lockChain();
                    ctx.engine->chain().addNode(node);
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

} // namespace gearboxfx
