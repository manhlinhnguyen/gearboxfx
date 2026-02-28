#pragma once
#include "AppContext.h"

namespace gearboxfx {

class TransportPanel {
public:
    void render(AppContext& ctx);

private:
    char m_loadedFile[512] = "";
};

} // namespace gearboxfx
