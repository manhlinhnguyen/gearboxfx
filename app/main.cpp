#include "EffectEngine.h"
#include "FileAudioIO.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <string>
#include <cstring>

using namespace gearboxfx;

static void printUsage(const char* prog) {
    std::cout <<
        "Usage: " << prog << " [options]\n"
        "\n"
        "Options:\n"
        "  --input  <path>   Input WAV file (required)\n"
        "  --preset <path>   Preset JSON file (required)\n"
        "  --output <path>   Output WAV file (file-to-file mode)\n"
        "  --play            Stream output to speakers (real-time mode)\n"
        "  --buffer <size>   DSP buffer size in frames (default: 256)\n"
        "  --bypass          Bypass all effects (pass-through)\n"
        "  --list-presets    Print registered effect types and exit\n"
        "  --help            Show this help\n"
        "\n"
        "Examples:\n"
        "  gearboxfx --input guitar.wav --preset presets/02_mild_overdrive.json --output out.wav\n"
        "  gearboxfx --input guitar.wav --preset presets/04_chorus_clean.json --play\n";
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);

    std::string inputPath;
    std::string outputPath;
    std::string presetPath;
    bool        playMode    = false;
    bool        bypassMode  = false;
    bool        listPresets = false;
    int         bufferSize  = 256;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--input") == 0 && i + 1 < argc)
            inputPath = argv[++i];
        else if (std::strcmp(argv[i], "--output") == 0 && i + 1 < argc)
            outputPath = argv[++i];
        else if (std::strcmp(argv[i], "--preset") == 0 && i + 1 < argc)
            presetPath = argv[++i];
        else if (std::strcmp(argv[i], "--play") == 0)
            playMode = true;
        else if (std::strcmp(argv[i], "--bypass") == 0)
            bypassMode = true;
        else if (std::strcmp(argv[i], "--buffer") == 0 && i + 1 < argc)
            bufferSize = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--list-presets") == 0)
            listPresets = true;
        else if (std::strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else {
            std::cerr << "Unknown option: " << argv[i] << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    EffectEngine engine;

    if (listPresets) {
        std::cout << "Registered effect types:\n";
        for (auto& t : engine.registry().registeredTypes())
            std::cout << "  " << t << "\n";
        return 0;
    }

    if (inputPath.empty()) {
        std::cerr << "Error: --input is required\n";
        printUsage(argv[0]);
        return 1;
    }

    if (presetPath.empty()) {
        std::cerr << "Error: --preset is required\n";
        printUsage(argv[0]);
        return 1;
    }

    if (!playMode && outputPath.empty()) {
        std::cerr << "Error: either --output or --play is required\n";
        printUsage(argv[0]);
        return 1;
    }

    // Prepare engine with default format (FileAudioIO will update after decoding)
    engine.prepare(48000.0, bufferSize);

    // Load preset
    if (!engine.loadPreset(presetPath)) {
        std::cerr << "Failed to load preset: " << presetPath << "\n";
        return 1;
    }

    engine.setBypass(bypassMode);

    // Configure file-sim
    FileSimConfig cfg;
    cfg.inputPath  = inputPath;
    cfg.outputPath = outputPath;
    cfg.bufferSize = static_cast<uint32_t>(bufferSize);
    cfg.mode = playMode ? FileSimMode::FileToSpeaker : FileSimMode::FileToFile;

    FileAudioIO io(engine, cfg);

    AudioFormat fmt;
    fmt.bufferSize = static_cast<uint32_t>(bufferSize);
    io.open(fmt);

    // Progress bar (file-to-file only)
    if (!playMode) {
        io.setProgressCallback([](float p) {
            int pct = static_cast<int>(p * 100.0f);
            std::cout << "\rProcessing: " << pct << "%" << std::flush;
        });
    }

    io.start();

    if (!playMode)
        std::cout << "\rProcessing: 100%\nDone! Output: " << outputPath << "\n";

    io.close();
    return 0;
}
