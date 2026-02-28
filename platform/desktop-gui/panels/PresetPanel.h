#pragma once
#include "AppContext.h"
#include <string>
#include <vector>

namespace gearboxfx {

class PresetPanel {
public:
    void render(AppContext& ctx);

private:
    struct PresetEntry {
        std::string name;
        std::string path;
    };

    void scanPresets(const std::string& dir);

    std::vector<PresetEntry> m_presets;
    int  m_selectedIdx = -1;
    bool m_needScan    = true;
    char m_saveName[256] = "new_preset";
};

} // namespace gearboxfx
