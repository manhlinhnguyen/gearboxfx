#include "GearBoxApp.h"

int main() {
    gearboxfx::GearBoxApp app;

    if (!app.init(1280, 720, "GearBox FX", "presets"))
        return 1;

    app.run();
    app.shutdown();
    return 0;
}
