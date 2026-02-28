#include "ChainPanel.h"
#include "EffectChain.h"
#include "EffectNode.h"
#include "effects/EffectNodeRegistry.h"
#include <imgui.h>
#include <imgui-knobs.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace gearboxfx {

namespace {

static constexpr float kBlockW   = 185.0f;
static constexpr float kKnobSize = 48.0f;

ImU32 categoryColor(const std::string& typeId) {
    if (typeId.compare(0, 8,  "dynamics")   == 0) return IM_COL32(50,  100, 200, 255); // blue
    if (typeId.compare(0, 2,  "eq")         == 0) return IM_COL32(200, 180,  30, 255); // yellow
    if (typeId.compare(0, 4,  "gain")       == 0) return IM_COL32(210, 120,  30, 255); // orange
    if (typeId.compare(0, 10, "modulation") == 0) return IM_COL32(140,  60, 200, 255); // purple
    if (typeId.compare(0, 6,  "output")     == 0) return IM_COL32( 90,  90, 100, 255); // gray
    if (typeId.compare(0, 4,  "time")       == 0) return IM_COL32( 30, 170, 160, 255); // teal
    return IM_COL32(80, 80, 80, 255);
}

std::string shortLabel(const std::string& typeId) {
    // "gain.overdrive" → "Overdrive", "modulation.pitch_shifter" → "Pitch Shifter"
    auto dot = typeId.find('.');
    std::string name = (dot != std::string::npos) ? typeId.substr(dot + 1) : typeId;

    std::string result;
    bool cap = true;
    for (char c : name) {
        if (c == '_') {
            result += ' ';
            cap = true;
        } else if (cap) {
            result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            cap = false;
        } else {
            result += c;
        }
    }
    return result;
}

const char* fmtForUnit(const std::string& unit) {
    if (unit == "dB") return "%.1f";
    if (unit == "ms") return "%.0f";
    if (unit == "Hz") return "%.0f";
    if (unit == "st") return "%.0f";
    return "%.2f";
}

} // anonymous namespace

void ChainPanel::render(AppContext& ctx) {
    ImGui::SetNextWindowPos(ImVec2(0, 155), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(1280, 565), ImGuiCond_Always);
    ImGui::Begin("Effects", nullptr,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_HorizontalScrollbar);

    const auto& nodes = ctx.engine->chain().nodes();
    const int   n     = static_cast<int>(nodes.size());

    // Deferred mutation state — mutations are applied after the render loop
    // so that the ImGui stack is always balanced when a button is clicked.
    bool chainMutated    = false;
    bool pendingMoveLeft = false, pendingMoveRight = false, pendingDelete = false;
    int  pendingIdx      = -1;
    std::shared_ptr<EffectNode> pendingNode;

    ImGui::BeginChild("##chain_scroll", ImVec2(0.0f, 0.0f), false,
        ImGuiWindowFlags_HorizontalScrollbar);

    for (int i = 0; i < n; ++i) {
        auto node = nodes[i]; // shared_ptr copy before any mutation

        if (i > 0) {
            ImGui::SameLine();
            ImGui::TextUnformatted(" \xe2\x86\x92 "); // UTF-8 "→"
            ImGui::SameLine();
        }

        ImGui::BeginGroup();
        ImGui::PushID(i);

        ImGui::BeginChild("##block", ImVec2(kBlockW, 0.0f), true);

        // ── Colored header strip ──────────────────────────────────────────
        {
            ImDrawList* dl  = ImGui::GetWindowDrawList();
            ImVec2      hp  = ImGui::GetCursorScreenPos();
            float       hw  = ImGui::GetContentRegionAvail().x;
            float       hh  = ImGui::GetTextLineHeight() + 6.0f;
            dl->AddRectFilled(hp, ImVec2(hp.x + hw, hp.y + hh),
                              categoryColor(node->typeId()), 2.0f);

            // Text positioned 4px in from the left, 3px down
            ImGui::SetCursorScreenPos(ImVec2(hp.x + 4.0f, hp.y + 3.0f));
            ImGui::TextUnformatted(shortLabel(node->typeId()).c_str());
        }

        // ON / OFF button — pinned to right side of header row
        ImGui::SameLine(kBlockW - 54.0f);
        bool enabled = node->isEnabled();
        if (enabled) ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(30, 160, 30, 255));
        else         ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(80, 80,  80, 255));
        if (ImGui::SmallButton(enabled ? " ON" : "OFF"))
            node->setEnabled(!enabled);
        ImGui::PopStyleColor();

        ImGui::Separator();

        // ── Parameters as knobs, 3 per row ───────────────────────────────
        const auto& defs   = node->paramDefs();
        const auto& values = node->paramValues();

        std::vector<std::string> pnames;
        pnames.reserve(defs.size());
        for (const auto& [nm, _] : defs)
            pnames.push_back(nm);
        std::sort(pnames.begin(), pnames.end());

        int col = 0;
        for (const auto& pname : pnames) {
            const auto& def = defs.at(pname);
            float v = values.at(pname);

            ImGui::PushID(pname.c_str());
            if (col > 0 && (col % 3) != 0)
                ImGui::SameLine();

            if (ImGuiKnobs::Knob(def.label.c_str(), &v,
                                  def.minValue, def.maxValue,
                                  0.0f, fmtForUnit(def.unit),
                                  ImGuiKnobVariant_Wiper, kKnobSize)) {
                ctx.engine->setParam(node->id() + "." + pname, v);
            }
            ImGui::PopID();
            ++col;
        }

        ImGui::Separator();

        // ── Reorder + delete buttons ─────────────────────────────────────
        ImGui::BeginDisabled(i == 0);
        bool ml = ImGui::SmallButton(" < ");
        ImGui::EndDisabled();

        ImGui::SameLine();

        ImGui::BeginDisabled(i == n - 1);
        bool mr = ImGui::SmallButton(" > ");
        ImGui::EndDisabled();

        ImGui::SameLine();
        bool del = ImGui::SmallButton(" X ");

        ImGui::EndChild();  // ##block
        ImGui::PopID();
        ImGui::EndGroup();

        // Record deferred mutation (ImGui stack is balanced at this point)
        if (ml && i > 0) {
            pendingMoveLeft = true; pendingIdx = i; pendingNode = node; chainMutated = true;
        } else if (mr && i < n - 1) {
            pendingMoveRight = true; pendingIdx = i; pendingNode = node; chainMutated = true;
        } else if (del) {
            pendingDelete = true; pendingIdx = i; pendingNode = node; chainMutated = true;
        }

        if (chainMutated) break;
    }

    // "+ Add Effect" button — inline after the last block
    if (n > 0) ImGui::SameLine();
    if (ImGui::Button("+ Add Effect"))
        ImGui::OpenPopup("add_effect_popup");

    if (ImGui::BeginPopup("add_effect_popup")) {
        auto types = ctx.engine->registry().registeredTypes();
        std::sort(types.begin(), types.end());

        for (const auto& typeId : types) {
            if (ImGui::MenuItem(typeId.c_str())) {
                auto newNode = ctx.engine->registry().create(typeId);
                if (newNode) {
                    std::string nid = typeId;
                    std::replace(nid.begin(), nid.end(), '.', '_');
                    nid += "_" + std::to_string(m_addCounter++);
                    newNode->setId(nid);
                    newNode->prepare(ctx.sampleRate, ctx.blockSize);

                    auto lock = ctx.audio->lockChain();
                    ctx.engine->chain().addNode(newNode);
                }
            }
        }
        ImGui::EndPopup();
    }

    ImGui::EndChild(); // ##chain_scroll

    // Execute deferred chain mutation (outside any child — stack is clean)
    if (chainMutated && pendingNode) {
        auto lock = ctx.audio->lockChain();
        if (pendingMoveLeft) {
            ctx.engine->chain().removeNode(pendingNode->id());
            ctx.engine->chain().insertNode(pendingIdx - 1, pendingNode);
        } else if (pendingMoveRight) {
            ctx.engine->chain().removeNode(pendingNode->id());
            ctx.engine->chain().insertNode(pendingIdx + 1, pendingNode);
        } else if (pendingDelete) {
            if (*ctx.selectedEffectId == pendingNode->id())
                ctx.selectedEffectId->clear();
            ctx.engine->chain().removeNode(pendingNode->id());
        }
    }

    ImGui::End();
}

} // namespace gearboxfx
