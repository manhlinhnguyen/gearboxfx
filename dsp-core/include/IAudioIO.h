#pragma once
#include <cstdint>

namespace gearboxfx {

struct AudioFormat {
    uint32_t sampleRate   = 48000;
    uint32_t numChannels  = 2;
    uint32_t bitDepth     = 24;
    uint32_t bufferSize   = 256;  // frames per block
};

class IAudioIO {
public:
    virtual ~IAudioIO() = default;

    // Called once before the audio loop starts.
    virtual bool open(const AudioFormat& fmt) = 0;

    // Start processing. Blocks until done (file mode) or until stop() (stream mode).
    virtual void start() = 0;

    // Signal the processing loop to stop.
    virtual void stop() = 0;

    // Called once after processing ends.
    virtual void close() = 0;

    virtual const AudioFormat& format() const = 0;
};

} // namespace gearboxfx
