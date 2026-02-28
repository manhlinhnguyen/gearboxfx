#include "effects/EffectNodeRegistry.h"
#include "effects/dynamics/NoiseGateNode.h"
#include "effects/dynamics/CompressorNode.h"
#include "effects/eq/EQNode.h"
#include "effects/gain/CleanBoostNode.h"
#include "effects/gain/OverdriveNode.h"
#include "effects/gain/DistortionNode.h"
#include "effects/modulation/ChorusNode.h"
#include "effects/modulation/FlangerNode.h"
#include "effects/modulation/PhaserNode.h"
#include "effects/modulation/PitchShifterNode.h"
#include "effects/modulation/TremoloNode.h"
#include "effects/output/VolumeNode.h"
#include "effects/time/DelayNode.h"
#include "effects/time/ReverbNode.h"

namespace gearboxfx {

void EffectNodeRegistry::registerAll() {
    reg<NoiseGateNode>    ("dynamics.noise_gate");
    reg<CompressorNode>   ("dynamics.compressor");
    reg<EQNode>           ("eq.parametric");
    reg<CleanBoostNode>   ("gain.clean_boost");
    reg<OverdriveNode>    ("gain.overdrive");
    reg<DistortionNode>   ("gain.distortion");
    reg<ChorusNode>       ("modulation.chorus");
    reg<FlangerNode>      ("modulation.flanger");
    reg<PhaserNode>       ("modulation.phaser");
    reg<PitchShifterNode> ("modulation.pitch_shifter");
    reg<TremoloNode>      ("modulation.tremolo");
    reg<VolumeNode>       ("output.volume");
    reg<DelayNode>        ("time.delay");
    reg<ReverbNode>       ("time.reverb");
}

} // namespace gearboxfx
