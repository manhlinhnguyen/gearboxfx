#pragma once
#include "AppContext.h"

namespace gearboxfx {

class ChainPanel {
public:
    void render(AppContext& ctx);

private:
    int m_addCounter = 0;  // monotonic counter for unique effect instance IDs
};

} // namespace gearboxfx
