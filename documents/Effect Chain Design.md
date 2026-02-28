## Nguyên tắc vàng: Signal Chain Guitar Effect

```
GUITAR
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  1. TUNER (tap off — không ảnh hưởng signal path)              │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  2. DYNAMICS (trước khi khuếch đại)                            │
│     Noise Gate → Compressor                                     │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  3. GAIN / DRIVE (tạo âm sắc chính)                            │
│     Clean Boost → Overdrive → Distortion → Fuzz                │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  4. EQ (định hình tần số sau khi đã clip)                       │
│     Bass / Mid / Treble / Parametric EQ                        │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  5. MODULATION (tác động lên âm thanh đã định hình)             │
│     Chorus → Flanger → Phaser → Tremolo → Pitch Shifter        │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  6. TIME / AMBIENCE (không gian — luôn đặt cuối)               │
│     Delay → Reverb                                              │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
┌─────────────────────────────────────────────────────────────────┐
│  7. OUTPUT VOLUME / LIMITER                                     │
└─────────────────────────────────────────────────────────────────┘
  │
  ▼
 AMP / PA / DAW
```

---

## Phân tích lý do từng vị trí

### 1. TUNER — Tap off đầu tiên (không nằm trong chain)

```
GUITAR ──┬──────────────────────────► Signal Chain ──►
         │
         └──► TUNER (parallel tap, đọc tín hiệu gốc)
```

**Lý do:**

- Tuner cần đọc **tín hiệu gốc sạch nhất** — chưa qua distortion, EQ, hay reverb
- Distortion tạo ra rất nhiều harmonic giả → pitch detection sai
- Reverb thêm các echo → nhiễu loạn autocorrelation của YIN algorithm
- Trong code: tuner chạy **song song** (không nối tiếp) với effect chain

---

### 2. NOISE GATE — Đầu tiên trong chain

**Lý do:**

- Cắt tạp âm **trước khi** nó được khuếch đại bởi Overdrive/Distortion
- Nếu đặt sau Distortion: tạp âm đã được nhân lên 20–50 lần, gate phải làm việc cực khó
- Nếu đặt sau Reverb: reverb tail sẽ bị cắt đột ngột → nghe rất xấu

```
SAI ❌:  Guitar → Distortion (noise ×50) → Noise Gate (quá muộn)
ĐÚNG ✅: Guitar → Noise Gate (cắt sạch) → Distortion
```

---

### 3. COMPRESSOR — Sau Noise Gate, trước Gain

**Lý do:**

- **Compressor trước Distortion:** tín hiệu đầu vào đồng đều hơn → distortion nhất quán hơn, sustain dài hơn. Đây là cách dùng classic (blues, rock)
- **Compressor sau Distortion:** kiểm soát dynamic của âm đã distort. Ít phổ biến hơn nhưng có thể dùng tùy mục đích

> **Trường hợp đặc biệt:** Một số setup dùng cả hai — compressor nhẹ trước gain, compressor mạnh sau gain (parallel compression)

---

### 4. GAIN STACK — Thứ tự trong nhóm

```
Clean Boost → Overdrive → Distortion → Fuzz
```

|Thứ tự|Âm thanh|Lý do|
|---|---|---|
|**Boost → Overdrive**|Overdrive đậm hơn, harmonics phong phú|Boost tăng input level cho overdrive|
|**Overdrive → Distortion**|Stacking layers, midrange dày|Classic "two-amp" stacking|
|**Fuzz cuối cùng**|Fuzz "nuốt" tín hiệu trước nó|Fuzz rất nhạy cảm với impedance|

> ⚠️ **Fuzz đặc biệt:** Fuzz thật (transistor germanium) phải đặt **đầu tiên trước mọi thứ**, ngay sau guitar, vì nó phụ thuộc vào impedance của pickup. Tuy nhiên trong **digital simulation** thì thứ tự này không còn quan trọng nữa.

---

### 5. EQ — Sau Gain, trước Modulation

**Lý do:**

- Distortion tạo ra tần số mới (harmonics) mà bạn muốn **điều chỉnh sau khi tạo ra**
- EQ trước distortion chỉ thay đổi "cái gì bị clip", không định hình tone cuối
- EQ sau distortion định hình **âm thanh cuối cùng** bạn nghe

```
SAI ❌:  Guitar → EQ (boost treble) → Distortion → Loa
         → Treble bị clip oan, thêm aliasing xấu

ĐÚNG ✅: Guitar → Distortion → EQ (boost treble) → Loa
         → Chỉ tăng treble của âm đã được xử lý xong
```

---

### 6. MODULATION — Sau EQ, trước Time

**Lý do:**

- Chorus/Flanger/Phaser tác động lên **tín hiệu hoàn chỉnh** (đã có tone đúng)
- Nếu Chorus đặt **trước** Distortion: mỗi voice của chorus bị distort riêng → tiếng lộn xộn, không rõ ràng
- Nếu Chorus đặt **sau** Delay: tiếng chorus bị nhân lên theo echo → thường không mong muốn

> **Ngoại lệ:** Vibrato (pitch modulation) đôi khi đặt trước gain để tạo hiệu ứng "warbly fuzz"

**Thứ tự trong nhóm modulation:**

```
Chorus → Flanger → Phaser → Tremolo → Pitch Shifter
```

- **Tremolo cuối nhóm:** vì tremolo cắt volume — nếu đặt trước chorus, chorus sẽ "fill in" những khoảng yên lặng của tremolo
- **Pitch Shifter:** nên đặt gần cuối nhóm modulation để pitch shift toàn bộ âm thanh đã xử lý

---

### 7. DELAY — Trước Reverb (luôn luôn)

**Lý do quan trọng nhất:**

```
ĐÚNG ✅: Tín hiệu → Delay → Reverb
         → Mỗi echo của delay sẽ có không gian reverb
         → Nghe như bạn đứng trong phòng và tiếng echo vang ra từ tường

SAI ❌:  Tín hiệu → Reverb → Delay
         → Toàn bộ "đám mây" reverb bị echo lại
         → Tiếng bùng nhùng, mất clarity hoàn toàn
```

**Hình dung:** Bạn hét vào tường (delay = echo), và phòng tạo reverb xung quanh tiếng hét đó. Không phải ngược lại.

---

### 8. REVERB — Cuối cùng

**Lý do:**

- Reverb tạo ra "không gian" — nó phải là lớp cuối cùng bao bọc toàn bộ âm thanh
- Bất cứ thứ gì đặt sau reverb sẽ phá vỡ cảm giác không gian đó

---

## Các thứ tự "phá vỡ quy tắc" có chủ ý

Đây là những setup **sáng tạo** mà guitarist dùng cố ý:

|Setup phi truyền thống|Âm thanh đạt được|
|---|---|
|Reverb → Distortion|"Shoegaze" — reverb bị saturate, tone rất dày và mơ hồ (My Bloody Valentine)|
|Delay → Distortion|Mỗi echo bị distort riêng, rất "dirty" và aggressive|
|Compressor → sau Reverb|Reverb tail được nén, sustain rất dài và "lush"|
|Flanger → trước Distortion|Flanging bị distort → "jet engine" sound|
|Tremolo → sau Reverb|Reverb tail không bị chặt — volume tremolo mượt hơn|

---

## Triển khai trong code — Default Chain Order

```cpp
// EffectNodeRegistry.h
// Thứ tự mặc định khi tạo preset từ template

struct EffectSlot {
    std::string category;
    int         defaultOrder;  // Thứ tự mặc định trong chain
    bool        allowReorder;  // Cho phép user kéo ra khỏi vị trí này không
};

static const std::vector<EffectSlot> DEFAULT_SIGNAL_CHAIN = {
    // Tuner: KHÔNG nằm trong chain, chạy parallel
    {"tuner",       -1,  false},

    // Chain chính
    {"dynamics",     0,  false},  // Noise Gate, Compressor — KHÔNG cho reorder ra ngoài group này
    {"gain",         1,  true },  // Overdrive, Distortion — có thể reorder trong group
    {"eq",           2,  true },  // EQ
    {"modulation",   3,  true },  // Chorus, Flanger, Phaser, Tremolo
    {"time",         4,  false},  // Delay → Reverb — KHÔNG cho đảo thứ tự Delay/Reverb
    {"output",       5,  false},  // Volume, Limiter — luôn cuối
};
```

```cpp
// Khi validate preset JSON — cảnh báo nếu thứ tự bất thường
class PresetValidator {
public:
    struct ValidationResult {
        bool        isValid;
        std::vector<std::string> warnings;  // Cảnh báo thứ tự không thông thường
        std::vector<std::string> errors;    // Lỗi nghiêm trọng
    };

    ValidationResult validate(const json& preset) {
        ValidationResult result;
        result.isValid = true;

        auto& chain = preset["effect_chain"];
        bool foundReverb = false;
        bool foundDelay  = false;
        bool foundGain   = false;

        for (auto& node : chain) {
            std::string type = node["type"];

            // Cảnh báo: Delay sau Reverb
            if (type == "time.delay" && foundReverb) {
                result.warnings.push_back(
                    "⚠️  Delay đặt sau Reverb — tiếng sẽ bùng nhùng. "
                    "Có thể đây là chủ ý (shoegaze style)?");
            }

            // Cảnh báo: Modulation trước Gain
            if (node["type"].get<std::string>().find("modulation") != std::string::npos
                && !foundGain) {
                result.warnings.push_back(
                    "⚠️  Modulation đặt trước Gain — chorus/flanger sẽ bị distort. "
                    "Đây có thể là hiệu ứng 'shoegaze' có chủ ý.");
            }

            if (type.find("time.reverb") != std::string::npos) foundReverb = true;
            if (type.find("time.delay")  != std::string::npos) foundDelay  = true;
            if (type.find("gain.")       != std::string::npos) foundGain   = true;
        }

        return result;
    }
};
```

---

## Tóm tắt thứ tự chuẩn — 1 hình duy nhất

```
INPUT (Guitar / WAV file)
    │
    ├──────────────────────────────► TUNER (parallel, không ảnh hưởng signal)
    │
    ▼
[NOISE GATE]      ← Cắt noise TRƯỚC khi khuếch đại
    │
    ▼
[COMPRESSOR]      ← Kiểm soát dynamic TRƯỚC khi distort
    │
    ▼
[CLEAN BOOST]     ┐
[OVERDRIVE]       ├── Gain stack: nhẹ → nặng
[DISTORTION]      ┘
    │
    ▼
[EQ]              ← Định hình tone SAU khi đã tạo ra harmonics
    │
    ▼
[CHORUS]          ┐
[FLANGER]         │
[PHASER]          ├── Modulation: tác động lên âm đã hoàn chỉnh
[TREMOLO]         │
[PITCH SHIFTER]   ┘
    │
    ▼
[DELAY]           ← Echo trước, để reverb "bao" lấy echo
    │
    ▼
[REVERB]          ← Không gian — LUÔN cuối cùng
    │
    ▼
[OUTPUT VOLUME]   ← Điều chỉnh volume tổng
    │
    ▼
OUTPUT (Loa / WAV file)
```

Bạn muốn tôi viết tiếp phần code **EffectChain với default ordering** và **drag-drop reorder validation**, hay bắt đầu code **FileAudioIO** để chạy thử với file WAV ngay?