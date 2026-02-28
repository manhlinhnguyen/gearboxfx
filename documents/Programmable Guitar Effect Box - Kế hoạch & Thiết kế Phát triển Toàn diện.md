**Phiên bản tài liệu:** 2.0  
**Ngày:** 2026-02-27  
**Loại:** Product Development Master Plan

---

# PHẦN I: TẦM NHÌN & CHIẾN LƯỢC SẢN PHẨM

## 1.1 Tầm nhìn sản phẩm (Product Vision)

> _"Một hộp hiệu ứng guitar điện thông minh, mã nguồn mở về preset, cho phép bất kỳ guitarist nào — từ nghiệp dư đến chuyên nghiệp — tự thiết kế, chia sẻ và tái sử dụng âm thanh của mình trên một nền tảng phần cứng nhỏ gọn, chi phí hợp lý."_

## 1.2 Tên sản phẩm & Định vị

|||
|---|---|
|**Tên sản phẩm**|**GearBox FX**|
|**Tagline**|_"Your Sound. Your Code."_|
|**Phân khúc**|Mid-range programmable multi-effects pedal|
|**Đối thủ tham chiếu**|Line 6 HX Stomp, Boss GT-1, Zoom MS-50G+|
|**Điểm khác biệt**|Preset hoàn toàn mở (JSON), cộng đồng chia sẻ, SDK mở|

## 1.3 Các nguyên tắc thiết kế cốt lõi

- **Modularity First:** Mọi thành phần — phần cứng lẫn phần mềm — đều có thể thay thế độc lập
- **Real-time is Non-negotiable:** Latency end-to-end < 10ms trong mọi điều kiện
- **Open Preset Ecosystem:** File preset JSON có thể đọc/chỉnh sửa bằng bất kỳ text editor nào
- **Incremental Hardware:** Bắt đầu bằng software simulation, tiến dần lên hardware thật mà không viết lại code

---

# PHẦN II: KIẾN TRÚC HỆ THỐNG TỔNG THỂ

## 2.1 Sơ đồ kiến trúc toàn hệ thống

```
┌─────────────────────────────────────────────────────────────────────┐
│                        GEARBOX FX SYSTEM                            │
│                                                                     │
│  ┌──────────┐    ┌──────────┐    ┌──────────────┐    ┌──────────┐  │
│  │  INPUT   │    │   ADC    │    │  DSP CORE    │    │   DAC    │  │
│  │  STAGE   │───▶│ 24bit/   │───▶│  (C++17)     │───▶│ 24bit/  │  │
│  │ Hi-Z 1MΩ │    │ 48kHz    │    │  Effect      │    │ 48kHz   │  │
│  └──────────┘    └──────────┘    │  Engine      │    └────┬─────┘  │
│                                  └──────┬───────┘         │        │
│                                         │                 │        │
│                               ┌─────────▼──────┐  ┌──────▼──────┐ │
│                               │  CONTROL MCU   │  │ OUTPUT STAGE│ │
│                               │  (ESP32)       │  │ Pre-Amp     │ │
│                               │  - BLE 5.0     │  │ EQ 3-band   │ │
│                               │  - USB-C       │  │ DI/XLR out  │ │
│                               │  - MIDI        │  └─────────────┘ │
│                               │  - Display     │                   │
│                               │  - Footswitches│                   │
│                               └───────┬────────┘                   │
└───────────────────────────────────────┼────────────────────────────┘
                                        │ BLE / USB
                          ┌─────────────▼──────────────┐
                          │      MOBILE APP             │
                          │  (Flutter / iOS / Android)  │
                          │  - Preset Editor            │
                          │  - Effect Chain Builder     │
                          │  - Cloud Sync               │
                          └─────────────┬───────────────┘
                                        │ HTTPS / REST
                          ┌─────────────▼──────────────┐
                          │      CLOUD BACKEND          │
                          │  - Preset Library           │
                          │  - User Community           │
                          │  - OTA Firmware             │
                          └────────────────────────────┘
```

## 2.2 Phân tầng phần mềm (Software Layer Architecture)

```
┌─────────────────────────────────────────────────┐
│              APPLICATION LAYER                   │
│  Mobile App  │  Desktop Sim  │  Web Preset Editor│
├──────────────┼───────────────┼───────────────────┤
│              COMMUNICATION LAYER                 │
│  BLE GATT    │  USB MIDI     │  REST API (Cloud) │
├─────────────────────────────────────────────────┤
│              DSP CORE (C++17) ← KHÔNG ĐỔI       │
│  EffectEngine │ EffectChain  │ PresetStore       │
│  Effect Nodes │ TunerProc    │ LooperProc        │
├─────────────────────────────────────────────────┤
│              HAL (Hardware Abstraction Layer)    │
│  LAPTOP: JUCE │ BELA: render()│ STM32: SAI DMA  │
├─────────────────────────────────────────────────┤
│              HARDWARE LAYER                      │
│  Soundcard   │ Bela Board    │ Custom PCB        │
└─────────────────────────────────────────────────┘
```

---

# PHẦN III: THIẾT KẾ PHẦN CỨNG CHI TIẾT

## 3.1 Sơ đồ khối phần cứng (Block Diagram)

```
                        ┌─────────────────────┐
GUITAR ──[6.35mm TS]──▶ │   INPUT BUFFER      │
                        │   JFET Hi-Z (≥1MΩ)  │
                        │   Low-noise preamp   │
                        │   Input clip LED     │
                        └──────────┬──────────┘
                                   │ Analog
                        ┌──────────▼──────────┐
                        │   ADC MODULE        │
                        │   WM8731 Codec      │
                        │   24-bit / 48kHz    │
                        │   Anti-alias filter │
                        └──────────┬──────────┘
                                   │ I2S Digital
                        ┌──────────▼──────────┐
                        │   DSP PROCESSOR     │◀── MicroSD (Presets/IR)
                        │   STM32H743ZIT6     │
                        │   480MHz Cortex-M7  │◀── QSPI Flash (Firmware)
                        │   1MB RAM + FPU     │
                        │   CMSIS-DSP lib     │──▶ OLED Display (I2C)
                        └──────────┬──────────┘
                                   │ I2S Digital
                        ┌──────────▼──────────┐
                        │   DAC MODULE        │
                        │   PCM5102A          │
                        │   24-bit / 48kHz    │
                        │   112dB DR          │
                        └──────────┬──────────┘
                                   │ Analog
                        ┌──────────▼──────────┐
                        │   OUTPUT STAGE      │──▶ [6.35mm TRS] LINE OUT
                        │   Op-amp buffer     │──▶ [XLR] DI OUT (balanced)
                        │   Volume pot        │
                        │   Bass/Mid/Treble   │
                        └─────────────────────┘

                        ┌─────────────────────┐
                        │   CONTROL MCU       │◀── Footswitch x4
                        │   ESP32-S3          │◀── Expression jack (TRS)
                        │   BLE 5.0 + WiFi    │◀── MIDI TRS In/Out
                        │   USB-C (OTG)       │◀── Knobs (ADC analog)
                        └─────────────────────┘
                               │     │
                          USB-C│     │BLE
                          ┌────▼┐  ┌─▼──────────┐
                          │ PC  │  │Smartphone  │
                          │ DAW │  │GearBox App │
                          └─────┘  └────────────┘
```

## 3.2 BOM — Bill of Materials (Danh sách linh kiện chính)

### 3.2.1 Core Components

|#|Linh kiện|Model|Chức năng|Đơn giá (ước tính)|
|---|---|---|---|---|
|1|DSP MCU|STM32H743ZIT6|Xử lý DSP chính, 480MHz Cortex-M7 + FPU|~$15|
|2|Control MCU|ESP32-S3-WROOM|BLE 5.0, WiFi, USB-C OTG, GPIO|~$4|
|3|ADC/Codec|WM8731L (Cirrus Logic)|ADC+DAC combo, 24-bit/48kHz, I2S|~$5|
|4|DAC|PCM5102A (TI)|High-quality DAC, 112dB DR|~$2|
|5|Display|SSD1306 OLED 1.3"|128×64, I2C, thông tin preset/tuner|~$3|
|6|Storage|MicroSD slot + 32GB card|Preset storage, IR files|~$5|
|7|Flash|W25Q128 (16MB QSPI)|Firmware storage, fast boot preset|~$1|

### 3.2.2 Analog Signal Path

|#|Linh kiện|Model|Chức năng|Đơn giá|
|---|---|---|---|---|
|8|Input JFET Buffer|2N5457 hoặc J201|Hi-Z impedance matching, guitar input|~$0.5|
|9|Op-Amp (signal path)|TL072 / OPA2134|Low-noise audio op-amp|~$1|
|10|Op-Amp (output)|NE5532 / OPA2604|Output stage, DI driver|~$1|
|11|DI Transformer|Lundahl LL1935|Balanced XLR output, galvanic isolation|~$8|
|12|Relay (True Bypass)|Omron G5V-2|True bypass relay, signal routing|~$1|
|13|Volume Potentiometer|Alps RK09 10kΩ audio|Volume control|~$2|
|14|Tone Potentiometers|Alps RK09 50kΩ x3|Bass/Mid/Treble EQ|~$6|

### 3.2.3 Controls & I/O

|#|Linh kiện|Model|Chức năng|Đơn giá|
|---|---|---|---|---|
|15|Footswitch chính|Taiway 250SP|True bypass toggle|~$2|
|16|Footswitch Preset Up/Down|Taiway 250SP x2|Preset navigation|~$4|
|17|Footswitch Tap Tempo/Looper|Taiway 250SP|Tap tempo, looper control|~$2|
|18|RGB LED|WS2812B x4|Status indicators|~$1|
|19|Input Jack|Neutrik NMJ6HFD2 6.35mm|Instrument input|~$3|
|20|Output Jack|Neutrik NMJ6HFD2 TRS|Line output|~$3|
|21|XLR Jack|Neutrik NC3FAH2|DI balanced output|~$4|
|22|Expression Jack|Stereo 6.35mm TRS|Expression pedal input|~$2|
|23|MIDI Jack|3.5mm TRS x2|MIDI In/Out (Type-A)|~$2|
|24|USB-C Connector|USB3.1 Type-C receptacle|Power + Data|~$1|
|25|DC Jack|2.1mm barrel jack|9V DC power|~$0.5|

### 3.2.4 Power Management

|#|Linh kiện|Model|Chức năng|Đơn giá|
|---|---|---|---|---|
|26|DC-DC Converter|LM2596|9V→5V buck cho digital section|~$1|
|27|LDO Analog|LP2950-5.0|9V→5V ultra-low-noise cho ADC/DAC|~$0.5|
|28|LDO 3.3V|AMS1117-3.3|5V→3.3V cho MCU, ESP32|~$0.3|
|29|EMI Filter|Common-mode choke + cap|Lọc nhiễu nguồn|~$1|
|30|Protection|TVS diode + polyfuse|Bảo vệ đầu vào 9V|~$0.5|

> **Tổng chi phí linh kiện ước tính: ~$80–100 USD/unit** (ở số lượng nhỏ)  
> Mục tiêu giá bán lẻ: **$249–299 USD**

## 3.3 Thiết kế PCB

### 3.3.1 Phân vùng PCB (PCB Partitioning)

```
┌─────────────────────────────────────────────────────┐
│                    PCB TOP VIEW                     │
│  ┌──────────────┐  ┌──────────────────────────────┐ │
│  │  ANALOG ZONE │  │       DIGITAL ZONE           │ │
│  │  (GND plane  │  │   (Separate GND plane,       │ │
│  │   isolated)  │  │    single-point tie to        │ │
│  │              │  │    analog GND at ADC)         │ │
│  │ Input Buffer │  │  STM32H743  │  ESP32-S3      │ │
│  │ ADC (analog) │  │  QSPI Flash │  USB-C         │ │
│  │ DAC (analog) │  │  MicroSD    │  BLE antenna   │ │
│  │ Output Stage │  │  OLED conn  │  (keep-out zone│ │
│  │ EQ / Preamp  │  │             │   near edge)   │ │
│  └──────────────┘  └──────────────────────────────┘ │
│                                                     │
│  ┌──────────────────────────────────────────────┐   │
│  │              POWER ZONE                       │   │
│  │  DC Jack │ Fuse │ LM2596 │ LDO_5V │ LDO_3V3  │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────┘
```

### 3.3.2 Quy tắc thiết kế PCB quan trọng

- **Tách biệt GND analog và GND digital** — nối một điểm duy nhất (star ground) tại chân GND của ADC
- **Audio trace:** độ rộng ≥ 0.3mm, tránh chạy song song với digital trace, bọc bằng GND guard trace
- **Decoupling capacitors:** 100nF ceramic sát mỗi chân VCC của IC, thêm 10µF bulk capacitor mỗi nguồn
- **BLE antenna:** vùng cấm (keep-out) không có copper pour bên dưới antenna ESP32
- **Crystal:** giữ trace ngắn, bao quanh bằng GND pour, cách xa mạch analog
- **Số lớp:** 4-layer PCB (Top Signal / GND / Power / Bottom Signal)

### 3.3.3 Kích thước & Vỏ hộp

```
┌──────────────────────────────────────┐
│  ┌────┐ ┌────┐ ┌────┐ ┌────┐        │
│  │VOL │ │BASS│ │MID │ │TREB│   OLED │
│  └────┘ └────┘ └────┘ └────┘  ┌────┐│
│                                │    ││
│  ●INPUT  ●OUTPUT  ●XLR         │    ││
│  ●EXP    ●MIDI    ●USB-C       └────┘│
│                                      │
│  ┌──────┐  ┌──────┐  ┌──────┐        │
│  │BYPASS│  │PRESET│  │ TAP  │  [LED] │
│  │  FS  │  │ UP/DN│  │TEMPO │        │
│  └──────┘  └──────┘  └──────┘        │
└──────────────────────────────────────┘
  ←────────── 180mm ──────────→
  ↕ 120mm
  Chiều cao vỏ: 60mm
  Vật liệu: Die-cast aluminum (Hammond 1590DD hoặc tương đương)
```

---

# PHẦN IV: THIẾT KẾ PHẦN MỀM CHI TIẾT

## 4.1 Cấu trúc toàn bộ repository

```
gearbox-fx/
│
├── firmware/                          # Code chạy trên thiết bị
│   ├── dsp-core/                      # ← MODULE DÙNG CHUNG (không đổi)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   ├── IAudioIO.h             # HAL interface
│   │   │   ├── EffectNode.h           # Base class effect
│   │   │   ├── EffectEngine.h         # Điều phối chính
│   │   │   ├── EffectChain.h          # Signal graph
│   │   │   ├── ParameterManager.h     # Quản lý params
│   │   │   ├── PresetStore.h          # Load/Save JSON
│   │   │   ├── TunerProcessor.h       # YIN pitch detection
│   │   │   ├── LooperProcessor.h      # Looper
│   │   │   └── effects/
│   │   │       ├── dynamics/
│   │   │       │   ├── NoiseGateNode.h
│   │   │       │   └── CompressorNode.h
│   │   │       ├── gain/
│   │   │       │   ├── OverdriveNode.h
│   │   │       │   ├── DistortionNode.h
│   │   │       │   └── AmpSimNode.h
│   │   │       ├── modulation/
│   │   │       │   ├── ChorusNode.h
│   │   │       │   ├── FlangerNode.h
│   │   │       │   └── PhaserNode.h
│   │   │       └── time/
│   │   │           ├── DelayNode.h
│   │   │           ├── ReverbNode.h
│   │   │           └── IRLoaderNode.h
│   │   └── src/
│   │       └── [implementations .cpp]
│   │
│   ├── platform/
│   │   ├── laptop/                    # JUCE wrapper (simulation)
│   │   │   ├── JuceAudioIO.h
│   │   │   ├── MainComponent.h/cpp    # JUCE GUI
│   │   │   └── CMakeLists.txt
│   │   ├── stm32/                     # STM32H7 HAL wrapper
│   │   │   ├── STM32AudioIO.h/cpp
│   │   │   ├── startup_stm32h743.s
│   │   │   └── CMakeLists.txt
│   │   └── bela/                      # Bela Board wrapper
│   │       ├── BelaAudioIO.cpp
│   │       └── CMakeLists.txt
│   │
│   ├── control/                       # ESP32 firmware (BLE, UI, MIDI)
│   │   ├── platformio.ini
│   │   ├── src/
│   │   │   ├── main.cpp
│   │   │   ├── BLEManager.h/cpp       # BLE GATT server
│   │   │   ├── MIDIManager.h/cpp      # MIDI In/Out
│   │   │   ├── DisplayManager.h/cpp   # OLED UI
│   │   │   ├── FootswitchManager.h    # Debounce, long-press
│   │   │   ├── PresetSelector.h/cpp   # Preset navigation
│   │   │   └── DSPBridge.h/cpp        # UART/SPI comm với STM32
│   │   └── lib/
│   │       ├── U8g2/                  # OLED library
│   │       └── NimBLE-Arduino/        # BLE library
│   │
│   └── presets/                       # Factory presets JSON
│       ├── clean_bright.json
│       ├── classic_rock_lead.json
│       ├── blues_overdrive.json
│       └── ...
│
├── desktop-sim/                       # Ứng dụng giả lập laptop
│   ├── CMakeLists.txt
│   ├── Source/
│   │   ├── Main.cpp
│   │   ├── MainComponent.h/cpp        # Màn hình chính
│   │   ├── EffectChainEditor.h/cpp    # Kéo thả effect chain
│   │   ├── PresetBrowser.h/cpp        # Quản lý preset
│   │   ├── ParameterPanel.h/cpp       # Chỉnh tham số
│   │   └── PluginProcessor.h/cpp      # VST3/AU plugin wrapper
│   └── assets/
│       └── effect-icons/
│
├── mobile-app/                        # Flutter app
│   ├── pubspec.yaml
│   ├── lib/
│   │   ├── main.dart
│   │   ├── core/
│   │   │   ├── models/
│   │   │   │   ├── preset.dart        # Preset data model
│   │   │   │   ├── effect_node.dart   # Effect node model
│   │   │   │   └── effect_param.dart  # Parameter model
│   │   │   ├── services/
│   │   │   │   ├── ble_service.dart   # BLE communication
│   │   │   │   ├── preset_service.dart
│   │   │   │   └── cloud_service.dart
│   │   │   └── state/                 # State management (Riverpod)
│   │   │       ├── device_state.dart
│   │   │       └── preset_state.dart
│   │   └── ui/
│   │       ├── screens/
│   │       │   ├── home_screen.dart
│   │       │   ├── preset_editor_screen.dart
│   │       │   ├── effect_library_screen.dart
│   │       │   ├── device_screen.dart
│   │       │   └── community_screen.dart
│   │       └── widgets/
│   │           ├── effect_node_card.dart
│   │           ├── chain_canvas.dart  # Kéo thả chain
│   │           ├── knob_widget.dart   # Knob UI ảo
│   │           └── tuner_widget.dart
│   └── test/
│
├── cloud-backend/                     # Backend API
│   ├── docker-compose.yml
│   ├── services/
│   │   ├── preset-api/               # NestJS / FastAPI
│   │   │   ├── src/
│   │   │   │   ├── presets/          # CRUD presets
│   │   │   │   ├── users/            # Auth
│   │   │   │   ├── community/        # Rating, comments
│   │   │   │   └── firmware/         # OTA management
│   │   │   └── Dockerfile
│   │   └── media-storage/            # S3-compatible (IR files)
│   └── infrastructure/
│       └── terraform/                 # AWS/GCP IaC
│
├── hardware/                          # Thiết kế PCB
│   ├── kicad/
│   │   ├── gearbox-fx.kicad_pro
│   │   ├── schematic/
│   │   ├── pcb/
│   │   └── bom/
│   ├── enclosure/
│   │   ├── 3d-model/                 # FreeCAD / Fusion 360
│   │   └── panel-layout.svg
│   └── datasheets/
│
├── docs/                              # Tài liệu
│   ├── technical-spec.md
│   ├── hardware-design-guide.md
│   ├── dsp-developer-guide.md         # Hướng dẫn viết effect mới
│   ├── preset-json-schema.md
│   └── api-reference/
│
└── tools/
    ├── preset-validator/              # Python script validate JSON
    ├── ir-converter/                  # Convert IR files
    └── firmware-flasher/              # CLI flash tool
```

## 4.2 DSP Core — Thiết kế chi tiết

### 4.2.1 Effect Node Registry (Factory Pattern)

```cpp
// Tất cả effect được đăng ký tại đây
// Thêm effect mới chỉ cần thêm 1 dòng, không sửa code khác
class EffectNodeRegistry {
public:
    static EffectNodeRegistry& instance();

    void registerAll() {
        // Dynamics
        reg<NoiseGateNode>  ("dynamics.noise_gate");
        reg<CompressorNode> ("dynamics.compressor");
        // Gain
        reg<OverdriveNode>  ("gain.overdrive");
        reg<DistortionNode> ("gain.distortion");
        reg<FuzzNode>       ("gain.fuzz");
        reg<AmpSimNode>     ("gain.amp_sim");
        // Modulation
        reg<ChorusNode>     ("modulation.chorus");
        reg<FlangerNode>    ("modulation.flanger");
        reg<PhaserNode>     ("modulation.phaser");
        reg<TremoloNode>    ("modulation.tremolo");
        reg<PitchShifterNode>("modulation.pitch_shifter");
        // Time
        reg<DelayNode>      ("time.delay");
        reg<ReverbNode>     ("time.reverb");
        reg<IRLoaderNode>   ("time.ir_loader");
    }

    std::shared_ptr<EffectNode> create(const std::string& type) {
        auto it = m_factory.find(type);
        if (it == m_factory.end()) return nullptr;
        return it->second();
    }

private:
    template<typename T>
    void reg(const std::string& typeId) {
        m_factory[typeId] = []() { return std::make_shared<T>(); };
    }

    std::unordered_map<std::string,
        std::function<std::shared_ptr<EffectNode>()>> m_factory;
};
```

### 4.2.2 JSON Preset Schema (chuẩn hóa đầy đủ)

```json
{
  "$schema": "https://gearboxfx.io/schemas/preset/v1.json",
  "preset_id": "550e8400-e29b-41d4-a716-446655440000",
  "format_version": "1.0",
  "name": "Classic Rock Lead",
  "author": "username",
  "description": "Warm overdrive into hall reverb, classic 70s lead tone",
  "tags": ["rock", "lead", "vintage", "warm"],
  "bpm": 120,
  "routing_mode": "serial",
  "effect_chain": [
    {
      "id": "ng_1",
      "type": "dynamics.noise_gate",
      "enabled": true,
      "params": { "threshold_db": -60.0, "release_ms": 100.0 }
    },
    {
      "id": "od_1",
      "type": "gain.overdrive",
      "enabled": true,
      "params": { "gain": 0.75, "tone": 0.6, "level": 0.8 }
    },
    {
      "id": "rev_1",
      "type": "time.reverb",
      "enabled": true,
      "params": {
        "mode": "hall",
        "size": 0.5,
        "decay": 0.6,
        "pre_delay_ms": 20.0,
        "mix": 0.25
      }
    }
  ],
  "expression_pedal": {
    "target_effect_id": "od_1",
    "target_param": "gain",
    "range_min": 0.0,
    "range_max": 1.0,
    "curve": "linear"
  },
  "output_eq": {
    "bass_db": 2.0,
    "mid_db": -1.0,
    "mid_freq_hz": 1000.0,
    "treble_db": 3.0
  },
  "output_volume": 0.85,
  "bypass_mode": "true_bypass",
  "created_at": "2026-02-27T10:00:00Z",
  "modified_at": "2026-02-27T10:00:00Z",
  "download_count": 0,
  "rating": 0.0
}
```

### 4.2.3 DSP Algorithm Map (các thuật toán cho từng effect)

|Effect|Thuật toán|Độ phức tạp CPU|Ghi chú|
|---|---|---|---|
|Noise Gate|Envelope follower + threshold comparator|Rất thấp|RMS window 10ms|
|Compressor|Peak/RMS detector + VCA gain computer|Thấp|Feed-forward topology|
|Overdrive|Arctan / Tanh soft clipper + IIR tone|Thấp|Antialias oversampling 2x|
|Distortion|Hard clip + asymmetric waveshaper|Thấp|Oversampling 4x|
|Amp Sim|Multi-stage tube model (Hammerstein)|Trung bình|Preamp + poweramp stages|
|Chorus|Multi-tap modulated delay line (LFO)|Trung bình|Stereo widening|
|Flanger|Short modulated delay + feedback|Thấp|Delay 0.1–10ms|
|Phaser|Cascaded all-pass filters (4–12 stages)|Trung bình|LFO-modulated cutoff|
|Tremolo|Amplitude modulation (LFO × signal)|Rất thấp|Sine/Square/Triangle LFO|
|Pitch Shifter|PSOLA hoặc Phase Vocoder|Cao|Latency 20–50ms|
|Delay|Circular buffer với interpolation|Thấp|Linear/Hermite interpolation|
|Reverb|Schroeder/FDN (Feedback Delay Network)|Trung bình|4–8 comb + 2–4 allpass|
|IR Loader|Convolution (FFT-based overlap-add)|Cao|Offline partition convolution|
|Tuner|YIN algorithm|Thấp|Chạy song song, không ảnh hưởng audio|
|Looper|Double-buffer circular recorder|Thấp|60s @ 48kHz = ~11MB RAM|

## 4.3 Firmware ESP32 (Control MCU) — Thiết kế chi tiết

### 4.3.1 BLE GATT Profile

```
GearBox FX BLE Service (UUID: 0xFB00)
│
├── Characteristic: PRESET_DATA (UUID: 0xFB01)
│   ├── Properties: Write (with response), Notify
│   ├── Mô tả: Nhận preset JSON từ app (chunked transfer nếu > 512 bytes)
│   └── Format: JSON UTF-8, chunked với header [chunk_id, total_chunks]
│
├── Characteristic: PRESET_SELECT (UUID: 0xFB02)
│   ├── Properties: Write, Notify
│   └── Mô tả: Chọn preset theo index hoặc UUID
│
├── Characteristic: PARAM_CONTROL (UUID: 0xFB03)
│   ├── Properties: Write (no response — tốc độ cao cho real-time)
│   └── Format: [effect_id:8, param_id:8, value:float32] = 10 bytes
│
├── Characteristic: DEVICE_STATUS (UUID: 0xFB04)
│   ├── Properties: Read, Notify
│   └── Format: JSON {bypass, preset_name, tuner_note, tuner_cents, bpm, battery}
│
├── Characteristic: TUNER_DATA (UUID: 0xFB05)
│   ├── Properties: Notify (10Hz update rate)
│   └── Format: [frequency:float32, note:uint8, cents:int8] = 6 bytes
│
└── Characteristic: OTA_FIRMWARE (UUID: 0xFB06)
    ├── Properties: Write
    └── Mô tả: Nhận firmware update binary (ESP32 OTA)
```

### 4.3.2 Communication Protocol giữa ESP32 và STM32

```
ESP32 ←──── UART (115200 baud) ────→ STM32H743
        (hoặc SPI 10MHz nếu cần tốc độ cao)

Message Frame Format:
┌────────┬────────┬──────────┬───────────────────┬──────────┐
│ START  │  MSG   │  LENGTH  │     PAYLOAD       │  CRC16   │
│ 0xAA55 │  TYPE  │ (2 bytes)│   (0-255 bytes)   │ (2 bytes)│
└────────┴────────┴──────────┴───────────────────┴──────────┘

Message Types:
  0x01: LOAD_PRESET    → Payload: preset JSON data
  0x02: SET_BYPASS     → Payload: [0/1]
  0x03: SET_PARAM      → Payload: [effect_id, param_id, float32]
  0x04: SET_VOLUME     → Payload: [float32]
  0x05: TAP_TEMPO      → Payload: [bpm: float32]
  0x10: STATUS_RESP    → Response: device status JSON
  0x11: TUNER_RESP     → Response: [freq, note, cents]
  0xFF: ACK/NACK       → Payload: [original_msg_type, 0=OK/1=ERR]
```

## 4.4 Mobile App — Thiết kế chi tiết (Flutter)

### 4.4.1 Kiến trúc App (Clean Architecture + Riverpod)

```
lib/
├── core/
│   ├── models/           # Pure Dart data classes
│   ├── repositories/     # Abstract interfaces
│   ├── services/         # BLE, Cloud, Local storage
│   └── utils/
├── features/
│   ├── device/           # Kết nối & quản lý thiết bị
│   ├── preset_editor/    # Soạn thảo preset
│   ├── effect_library/   # Thư viện effect
│   ├── community/        # Chia sẻ preset
│   └── settings/
└── shared/
    └── widgets/          # Reusable widgets
```

### 4.4.2 Màn hình chính & Navigation

```
Bottom Navigation:
┌─────────┬──────────┬──────────┬──────────┬──────────┐
│  Device │  Presets │  Editor  │Community │ Settings │
│  🎛️     │  📋      │  ✏️      │  🌐      │  ⚙️     │
└─────────┴──────────┴──────────┴──────────┴──────────┘

Device Screen:
  - Status card: tên preset, bypass state, battery
  - Live tuner display (chromatic)
  - Quick preset switcher (swipe)
  - BLE connection status + pair button

Preset Editor Screen:
  - Visual effect chain (horizontal scroll, drag to reorder)
  - Tap effect node → open parameter panel
  - Add effect button → Effect Library picker
  - Preset name, tags, BPM editor
  - Save / Send to Device / Export JSON buttons

Effect Library Screen:
  - Grid view, grouped by category
  - Search & filter
  - Tap → Preview description + default params

Community Screen:
  - Browse/search public presets
  - Download to My Presets
  - Rate & comment
  - Share via QR Code / link
```

## 4.5 Cloud Backend — Thiết kế chi tiết

### 4.5.1 API Endpoints (REST)

```
BASE URL: https://api.gearboxfx.io/v1

AUTH:
  POST   /auth/register
  POST   /auth/login
  POST   /auth/refresh

PRESETS:
  GET    /presets              → List public presets (paginated, filterable)
  GET    /presets/:id          → Get preset detail
  POST   /presets              → Upload new preset (auth required)
  PUT    /presets/:id          → Update preset (owner only)
  DELETE /presets/:id          → Delete preset (owner only)
  GET    /presets/:id/download → Download JSON file (increments counter)
  POST   /presets/:id/rate     → Rate preset 1-5 stars

USER PRESETS:
  GET    /users/me/presets     → My presets (public + private)
  GET    /users/me/favorites   → Favorited presets

FIRMWARE:
  GET    /firmware/latest      → Latest firmware version info
  GET    /firmware/:version    → Download firmware binary

IR FILES:
  GET    /ir-files             → Browse IR file library
  POST   /ir-files             → Upload IR file
  GET    /ir-files/:id         → Download IR file
```

### 4.5.2 Database Schema (PostgreSQL)

```sql
-- Users
CREATE TABLE users (
  id          UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  username    VARCHAR(50) UNIQUE NOT NULL,
  email       VARCHAR(255) UNIQUE NOT NULL,
  created_at  TIMESTAMPTZ DEFAULT NOW()
);

-- Presets
CREATE TABLE presets (
  id              UUID PRIMARY KEY DEFAULT gen_random_uuid(),
  author_id       UUID REFERENCES users(id),
  name            VARCHAR(100) NOT NULL,
  description     TEXT,
  tags            TEXT[],
  preset_data     JSONB NOT NULL,       -- Full preset JSON
  is_public       BOOLEAN DEFAULT true,
  download_count  INTEGER DEFAULT 0,
  avg_rating      NUMERIC(3,2) DEFAULT 0,
  format_version  VARCHAR(10) DEFAULT '1.0',
  created_at      TIMESTAMPTZ DEFAULT NOW(),
  modified_at     TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_presets_tags ON presets USING GIN(tags);
CREATE INDEX idx_presets_public ON presets(is_public, avg_rating DESC);

-- Ratings
CREATE TABLE preset_ratings (
  preset_id   UUID REFERENCES presets(id),
  user_id     UUID REFERENCES users(id),
  rating      SMALLINT CHECK (rating BETWEEN 1 AND 5),
  PRIMARY KEY (preset_id, user_id)
);
```

---

# PHẦN V: KẾ HOẠCH PHÁT TRIỂN (ROADMAP)

## 5.1 Tổng quan các giai đoạn

```
2026          Q1            Q2            Q3            Q4
         ├─────────────┼─────────────┼─────────────┼─────────────┤
Phase 1  ████████████
(Sim)
Phase 2              ████████████████
(Alpha HW)
Phase 3                               ████████████████
(Beta)
Phase 4                                               █████████████
(Release)
```

---

## 5.2 Phase 1 — Software Simulation (Tháng 1–3)

**Mục tiêu:** Hoàn thiện DSP Core + Desktop Sim App + Mobile App MVP  
**Không cần phần cứng chuyên dụng — chạy hoàn toàn trên laptop**

### Sprint 1 (Tuần 1–2): DSP Core Infrastructure

- [ ]  Setup CMake project, integrate `nlohmann/json`
- [ ]  Implement `IAudioIO`, `EffectNode` base class
- [ ]  Implement `EffectChain` với ping-pong buffer
- [ ]  Implement `EffectEngine` với atomic bypass
- [ ]  Implement `EffectNodeRegistry` (factory pattern)
- [ ]  Unit tests cho từng component (Google Test)

### Sprint 2 (Tuần 3–4): Core Effects (set 1)

- [ ]  `NoiseGateNode` — envelope follower + threshold
- [ ]  `OverdriveNode` — arctan soft clipper + tone filter
- [ ]  `DistortionNode` — hard clipper + asymmetric waveshaper
- [ ]  `DelayNode` — circular buffer + linear interpolation + BPM sync
- [ ]  `ReverbNode` — Schroeder network (4 comb + 2 allpass)
- [ ]  Test nghe âm thanh trực tiếp qua JUCE standalone app

### Sprint 3 (Tuần 5–6): Core Effects (set 2) + Preset System

- [ ]  `CompressorNode` — feed-forward RMS compressor
- [ ]  `ChorusNode` — multi-tap modulated delay
- [ ]  `PhaserNode` — cascaded all-pass filters
- [ ]  `PresetStore` — load/save JSON từ file system
- [ ]  `ParameterManager` — thread-safe param update
- [ ]  Implement `TunerProcessor` (YIN algorithm)

### Sprint 4 (Tuần 7–8): Desktop Sim App (JUCE GUI)

- [ ]  Main window layout
- [ ]  `EffectChainEditor` — drag & drop nodes, kéo thả sắp xếp
- [ ]  `ParameterPanel` — knob/slider cho từng effect
- [ ]  `PresetBrowser` — load/save/list presets
- [ ]  Bypass toggle button
- [ ]  Tuner display
- [ ]  Build như VST3 plugin (test trong DAW)

### Sprint 5 (Tuần 9–10): Mobile App MVP (Flutter)

- [ ]  Project setup, Riverpod state management
- [ ]  Preset data models, local storage (Hive/Isar)
- [ ]  Preset Editor UI (effect chain canvas, drag-drop)
- [ ]  Knob widget, parameter editor
- [ ]  BLE service stub (mock device)
- [ ]  Import/Export JSON preset

### Sprint 6 (Tuần 11–12): Integration & Testing

- [ ]  BLE communication thực (laptop ↔ phone qua ESP32 dev board)
- [ ]  End-to-end test: edit preset trên app → gửi BLE → load vào DSP
- [ ]  Latency measurement trên laptop
- [ ]  Document DSP developer guide

**Deliverable Phase 1:**

- ✅ Desktop app chạy được, xử lý guitar real-time qua soundcard laptop
- ✅ Mobile app chỉnh sửa và gửi preset qua BLE
- ✅ ≥ 10 effect nodes hoạt động ổn định
- ✅ Latency < 15ms trên laptop

---

## 5.3 Phase 2 — Alpha Hardware (Tháng 4–6)

**Mục tiêu:** Prototype phần cứng thực, port DSP Core sang STM32

### Sprint 7–8: Schematic & PCB Design

- [ ]  Vẽ schematic trên KiCad: Input stage, ADC (WM8731), DAC (PCM5102A)
- [ ]  Schematic: STM32H743, ESP32-S3, power management
- [ ]  Schematic: I/O jacks, footswitches, pots, OLED
- [ ]  Review & DRC (Design Rule Check)
- [ ]  PCB layout 4-layer, phân vùng analog/digital
- [ ]  Gửi Gerber file đặt làm PCB (JLCPCB/PCBWay)

### Sprint 9–10: STM32 Firmware Port

- [ ]  Setup STM32CubeIDE + CMake toolchain (arm-none-eabi-gcc)
- [ ]  Cấu hình SAI (Serial Audio Interface) + DMA cho I2S audio
- [ ]  Port `IAudioIO` → `STM32AudioIO` (gắn vào SAI DMA callback)
- [ ]  Port `PresetStore` → dùng FatFS trên MicroSD
- [ ]  Tối ưu CMSIS-DSP cho các thuật toán nặng (FFT, IIR filter)
- [ ]  Đo latency thực tế trên STM32

### Sprint 11–12: ESP32 Firmware

- [ ]  BLE GATT server hoàn chỉnh (NimBLE)
- [ ]  UART bridge giao tiếp với STM32
- [ ]  OLED display (U8g2): preset name, tuner, bypass status
- [ ]  Footswitch debounce, long-press detection
- [ ]  Potentiometer ADC reading → gửi sang STM32
- [ ]  MIDI In/Out (TRS)

**Deliverable Phase 2:**

- ✅ Prototype PCB hoạt động với toàn bộ tín hiệu audio path
- ✅ Latency end-to-end < 10ms trên STM32
- ✅ BLE hoạt động ổn định, preset sync từ phone sang hardware
- ✅ OLED hiển thị đúng thông tin

---

## 5.4 Phase 3 — Beta (Tháng 7–9)

**Mục tiêu:** Hoàn thiện tính năng, tối ưu chất lượng âm thanh, cloud backend

### Sprint 13–14: Advanced Features

- [ ]  `IRLoaderNode` — FFT convolution (overlap-add)
- [ ]  `LooperProcessor` — double buffer, overdub, undo
- [ ]  `AmpSimNode` — tube amp simulation (Hammerstein model)
- [ ]  Expression pedal input → real-time param modulation
- [ ]  Tap Tempo → BPM sync cho Delay, Tremolo, Chorus

### Sprint 15–16: Cloud Backend

- [ ]  Setup NestJS + PostgreSQL + S3-compatible storage
- [ ]  CRUD API cho presets, users, ratings
- [ ]  JWT auth, rate limiting
- [ ]  IR file upload/download
- [ ]  OTA firmware management endpoint
- [ ]  Deploy trên AWS / Railway / Render

### Sprint 17–18: Mobile App hoàn chỉnh

- [ ]  Community screen: browse, download, rate preset
- [ ]  QR Code share/import
- [ ]  OTA firmware update flow
- [ ]  Remote real-time control (smartphone → BLE → STM32)
- [ ]  Chromatic tuner màn hình full với note display

### Sprint 19–20: Beta Testing & Audio QA

- [ ]  Noise floor measurement, THD+N test
- [ ]  A/B comparison với các pedal thương mại tham chiếu
- [ ]  Stress test: chạy effect chain 8 nodes liên tục 24h
- [ ]  Beta test với 10–20 guitarist thực tế
- [ ]  Fix bugs từ feedback

**Deliverable Phase 3:**

- ✅ Tất cả tính năng chính hoạt động
- ✅ Cloud backend stable
- ✅ Audio quality đạt tiêu chuẩn (THD+N < 0.01%, noise floor < -90dBFS)

---

## 5.5 Phase 4 — Production Release (Tháng 10–12)

### Sprint 21–22: PCB Rev B + Enclosure

- [ ]  Sửa lỗi PCB từ prototype, tối ưu layout
- [ ]  Thiết kế vỏ aluminum + panel silk-screen
- [ ]  Sản xuất thử 10–20 units
- [ ]  OLED silk-screen label cho các nút/jack

### Sprint 23–24: Production Readiness

- [ ]  Firmware v1.0 release, ký số binary
- [ ]  Factory preset pack (30+ presets)
- [ ]  SDK documentation cho developer viết effect mới
- [ ]  App phát hành lên App Store + Google Play
- [ ]  Trang web sản phẩm + community forum

---

# PHẦN VI: CÔNG NGHỆ & CÔNG CỤ PHÁT TRIỂN

## 6.1 Stack kỹ thuật đầy đủ

|Tầng|Công nghệ|Lý do chọn|
|---|---|---|
|DSP Core|C++17, CMake|Hiệu năng, portability, STM32 compatible|
|DSP Libraries|CMSIS-DSP, nlohmann/json|Tối ưu ARM, JSON header-only|
|Desktop Sim|JUCE 7 (C++)|Industry standard, VST3/AU/Standalone|
|STM32 Firmware|STM32CubeHAL, FreeRTOS, arm-gcc|Official support, RTOS cho multi-task|
|ESP32 Firmware|PlatformIO, Arduino framework, NimBLE|Dễ phát triển BLE|
|Mobile App|Flutter 3 (Dart)|Cross-platform iOS+Android, 1 codebase|
|State Management|Riverpod 2|Reactive, testable|
|Cloud Backend|NestJS (TypeScript) + PostgreSQL|Type-safe, scalable|
|Cloud Infra|Docker + Railway/Render (dev), AWS (prod)|Chi phí thấp khi start|
|PCB Design|KiCad 7|Miễn phí, professional, open-source|
|3D Enclosure|Fusion 360 / FreeCAD|Thiết kế vỏ hộp|
|Version Control|Git + GitHub|Monorepo|
|CI/CD|GitHub Actions|Auto build + test firmware, app|
|Testing|Google Test (C++), Jest (Backend), Flutter Test||

## 6.2 Development Environment Setup

```bash
# 1. Clone repo
git clone https://github.com/yourname/gearbox-fx.git
cd gearbox-fx

# 2. DSP Core + Desktop Sim (requires JUCE, CMake)
cd desktop-sim
cmake -B build -DPLATFORM=laptop -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/GearBoxFX_Standalone   # Chạy standalone app

# 3. STM32 Firmware (requires arm-none-eabi-gcc)
cd firmware
cmake -B build-stm32 -DPLATFORM=stm32 \
      -DCMAKE_TOOLCHAIN_FILE=platform/stm32/toolchain-stm32h7.cmake
cmake --build build-stm32

# 4. ESP32 Firmware (requires PlatformIO)
cd firmware/control
pio run --target upload

# 5. Mobile App (requires Flutter 3.x)
cd mobile-app
flutter pub get
flutter run

# 6. Cloud Backend (requires Docker)
cd cloud-backend
docker-compose up -d
```

---

# PHẦN VII: QUẢN LÝ RỦI RO

## 7.1 Ma trận rủi ro

|Rủi ro|Xác suất|Mức độ|Biện pháp giảm thiểu|
|---|---|---|---|
|**Latency STM32 > 10ms**|Trung bình|Nghiêm trọng|Tối ưu DMA buffer size; dự phòng sang Bela Board|
|**Nhiễu audio (noise/hum)**|Cao|Nghiêm trọng|Phân vùng GND kỹ; dùng LDO riêng cho analog|
|**BLE không ổn định**|Thấp|Trung bình|Dùng NimBLE (ổn định hơn Arduino BLE); test kỹ chunked transfer|
|**PCB lỗi lần đầu**|Cao|Trung bình|Thiết kế test point; dự trù ít nhất PCB Rev B|
|**JUCE license cost**|Thấp|Thấp|JUCE miễn phí cho revenue < $50k/năm|
|**Flutter BLE trên iOS**|Trung bình|Trung bình|Dùng flutter_blue_plus, test sớm trên iOS|
|**STM32 chip shortage**|Thấp|Cao|Chuẩn bị phương án thay thế: iMX RT1062 (Teensy 4.1)|

## 7.2 Kế hoạch dự phòng phần cứng

Nếu STM32H743 gặp vấn đề:

- **Phương án B:** Teensy 4.1 (IMXRT1062, 600MHz) — cộng đồng audio lớn, dễ lập trình hơn
- **Phương án C:** Bela Board — latency tốt nhất, nhưng chi phí cao hơn (~$200 board)

---

# PHẦN VIII: CHỈ SỐ THÀNH CÔNG (KPIs)

|KPI|Phase 1|Phase 2|Phase 4|
|---|---|---|---|
|Audio Latency|< 15ms (laptop)|< 10ms (STM32)|< 7ms|
|THD+N|N/A|< 0.05%|< 0.01%|
|Noise Floor|N/A|< -85 dBFS|< -95 dBFS|
|Số Effect Nodes|≥ 10|≥ 15|≥ 25|
|Preset Load Time|< 100ms|< 50ms|< 30ms|
|BLE Transfer Speed|5KB preset < 2s|5KB preset < 1s|5KB preset < 0.5s|
|App Crash Rate|< 5%|< 1%|< 0.1%|
|Battery Life (nếu có)|N/A|≥ 4h|≥ 6h|