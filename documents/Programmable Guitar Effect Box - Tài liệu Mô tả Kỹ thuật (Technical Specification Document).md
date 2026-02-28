**Phiên bản:** 1.0  
**Loại thiết bị:** Hộp hiệu ứng guitar điện có thể lập trình được  
**Mô hình triển khai:** Embedded Hardware + Mobile Application

---

## 1. Tổng quan hệ thống (System Overview)

Thiết bị là một **hộp hiệu ứng guitar điện đa năng, có thể lập trình được (Programmable Multi-Effects Pedal)**, cho phép người dùng tự thiết kế, lưu trữ và áp dụng các chuỗi hiệu ứng âm thanh (effect chain) thông qua ứng dụng smartphone. Kiến trúc hệ thống tuân theo pipeline xử lý tín hiệu số chuẩn:

```
Guitar → [Input Stage] → [ADC] → [DSP Core] → [DAC] → [Output Stage] → Amplifier/Speaker
```

Thiết bị được thiết kế hướng đến ba nhóm người dùng chính:

- **Guitarist nghiệp dư** muốn khám phá âm thanh linh hoạt với chi phí hợp lý
- **Guitarist chuyên nghiệp** cần một pedalboard gọn nhẹ, linh hoạt và có thể lập trình
- **Nhà phát triển âm thanh** muốn thử nghiệm và chia sẻ các hiệu ứng tùy chỉnh

---

## 2. Kiến trúc phần cứng (Hardware Architecture)

### 2.1 Module Input Stage (Tầng đầu vào)

**Chức năng:** Tiếp nhận và chuẩn hóa tín hiệu analog từ guitar điện trước khi đưa vào ADC.

**Thành phần:**

- **Input Buffer & Impedance Matching:** Mạch đệm JFET hoặc Op-Amp với input impedance ≥ 1MΩ để tương thích với pickups thụ động (passive) và chủ động (active) của guitar điện. Đây là bước bắt buộc để tránh tổn thất tín hiệu (tone suck) và đảm bảo tính tương thích với mọi loại guitar.
- **Noise Gate / Low-Noise Preamp:** Khuếch đại nhẹ tín hiệu đầu vào (gain ~10–20 dB) và lọc nhiễu nền (hum, hiss) trước khi ADC xử lý.
- **Input Level Indicator:** LED đơn giản hoặc bargraph để hiển thị mức tín hiệu đầu vào, tránh clipping ở ADC.
- **Cổng vật lý:** Giắc cắm 6.35mm (1/4") TS Mono (tiêu chuẩn instrument cable).

### 2.2 Module ADC (Analog-to-Digital Converter)

**Chức năng:** Chuyển đổi tín hiệu audio analog sang dạng số để DSP xử lý.

**Thông số kỹ thuật:**

|Thông số|Giá trị|
|---|---|
|Sample Rate|48 kHz (hỗ trợ 44.1 kHz tùy chế độ)|
|Bit Depth|24-bit|
|Dynamic Range|≥ 100 dB|
|THD+N|< 0.01%|
|Anti-aliasing Filter|Low-pass filter tích hợp (cutoff ~22 kHz)|

**Gợi ý IC:** PCM4201 (Texas Instruments), AKM AK5552, hoặc tích hợp codec audio chuyên dụng như WM8731 (Wolfson/Cirrus Logic).

### 2.3 Module DSP Core (Bộ xử lý tín hiệu số)

**Chức năng:** Tiếp nhận luồng audio số từ ADC, xử lý theo effect preset đã chọn, và truyền tín hiệu đã xử lý sang DAC theo thời gian thực.

**Yêu cầu kỹ thuật tối thiểu:**

- **Latency (độ trễ) end-to-end:** < 10ms (ngưỡng người chơi guitar cảm nhận được)
- **Xử lý thời gian thực:** Bắt buộc phải có khả năng xử lý real-time, không được phép có jitter (dao động độ trễ)

**Lựa chọn nền tảng phần cứng:**

|Nền tảng|Ưu điểm|Nhược điểm|Phù hợp|
|---|---|---|---|
|**Bela Board** (BeagleBone + PRU)|Latency cực thấp (~1ms), dedicated audio|Cộng đồng nhỏ|✅ Tối ưu nhất|
|**Elk Audio OS** trên Raspberry Pi CM4|Hệ sinh thái rộng, RTOS audio|Chi phí cao hơn|✅ Chuyên nghiệp|
|**STM32H7 + FPU** (bare-metal)|Chi phí thấp, tiêu thụ điện thấp|Lập trình phức tạp|✅ Tối ưu chi phí|
|**Raspberry Pi 4** + Linux tiêu chuẩn|Dễ lập trình|Latency cao do kernel không RTOS|⚠️ Chỉ dùng prototype|

> **Khuyến nghị:** Sử dụng **Bela Board** hoặc **STM32H743** kết hợp với CMSIS-DSP library cho sản phẩm thực tế; dùng **Raspberry Pi 4 + PREEMPT_RT kernel patch** cho giai đoạn prototyping và phát triển.

**Phần mềm DSP:**

- Ngôn ngữ: C/C++ (hiệu năng cao) hoặc **Pure Data (Pd)** / **FAUST DSP language** cho khả năng lập trình hiệu ứng nhanh hơn
- Kiến trúc xử lý: Block-based processing với buffer size nhỏ (64–256 samples) để đảm bảo latency thấp
- Effect Engine: Xử lý tín hiệu theo đồ thị (signal graph), các effect node kết nối theo thứ tự chuỗi (effect chain)

### 2.4 Module DAC (Digital-to-Analog Converter)

**Chức năng:** Chuyển đổi tín hiệu số đã xử lý từ DSP sang tín hiệu analog để xuất ra thiết bị khuếch đại.

**Thông số kỹ thuật:**

|Thông số|Giá trị|
|---|---|
|Sample Rate|48 kHz|
|Bit Depth|24-bit|
|Dynamic Range|≥ 100 dB|
|Output Impedance|≤ 600Ω (line-level output)|

**Gợi ý IC:** PCM5102A (Texas Instruments), WM8741 (Cirrus Logic).

### 2.5 Module Output Stage & Pre-Amp tích hợp

**Chức năng:** Điều chỉnh tín hiệu analog đầu ra trước khi truyền đến amplifier hoặc PA system.

**Tính năng:**

- **Volume Control:** Núm điều chỉnh âm lượng đầu ra (analog potentiometer, dải 0–100%)
- **3-Band Equalizer (Tone Control):**
    - **Bass:** ±12 dB tại 80 Hz
    - **Mid:** ±12 dB tại 1 kHz (sweepable 500 Hz – 2 kHz)
    - **Treble:** ±12 dB tại 8 kHz
- **Cabinet Simulation (Cab Sim) / DI Output:** Đầu ra XLR cân bằng (balanced) mô phỏng âm thanh qua loa cabinet bằng Impulse Response (IR), dùng để kết nối trực tiếp vào bàn mixer hoặc audio interface.
- **Line Output:** Giắc cắm 6.35mm (1/4") TRS cho kết nối unbalanced line-level.

### 2.6 Module Bluetooth & Connectivity

**Chức năng:** Kết nối không dây với smartphone để quản lý và nạp effect preset.

**Thông số:**

- **Bluetooth:** BLE 5.0 (Bluetooth Low Energy) — tiêu thụ điện thấp, đủ băng thông cho việc truyền dữ liệu preset
- **Module gợi ý:** ESP32 (tích hợp BLE + Wi-Fi), Nordic nRF52840
- **Giao thức:** Custom GATT Profile qua BLE, hoặc MIDI over BLE (tiêu chuẩn Apple MIDI-BLE)
- **Chức năng:** Gửi/nhận preset JSON, cập nhật firmware OTA (Over-The-Air), điều khiển tham số real-time từ smartphone
- **Bảo mật:** Pairing có xác nhận, mã hóa BLE tiêu chuẩn

**Tùy chọn bổ sung:**

- **USB Type-C:** Nạp điện + truyền dữ liệu preset khi kết nối với máy tính (USB Audio Class 2.0 — dùng thiết bị như một audio interface luôn)
- **MIDI DIN 5-pin (In/Out/Thru):** Tích hợp MIDI hardware để tương thích với các thiết bị MIDI khác (keyboard, MIDI controller, DAW)
- **MIDI over TRS:** Cổng 3.5mm TRS MIDI (tiêu chuẩn MIDI TRS Type-A) tiết kiệm không gian hơn DIN

### 2.7 Module Display

**Chức năng:** Hiển thị thông tin trạng thái thiết bị và preset đang hoạt động.

**Lựa chọn:**

- **OLED 128×64 (0.96" hoặc 1.3"):** Hiển thị tên preset, trạng thái bypass, BPM (tap tempo), tuner, trạng thái kết nối Bluetooth — chi phí thấp, độ tương phản cao
- **TFT LCD màu 2.4":** Hiển thị signal chain dạng đồ họa đơn giản, màu sắc theo loại effect — trực quan hơn

### 2.8 Module Power Management

**Chức năng:** Cung cấp nguồn điện ổn định và sạch cho toàn bộ hệ thống.

**Nguồn cấp:**

- **DC Adapter:** 9V DC Center-negative (tiêu chuẩn pedalboard phổ biến nhất), 500mA – 1A
- **Pin sạc (tùy chọn):** Li-Ion 18650 hoặc Li-Po tích hợp, với mạch quản lý pin (BMS) và sạc USB-C (PD), thời lượng pin ≥ 6 giờ
- **Voltage Regulation:** Bộ lọc nguồn (EMI filter) và LDO regulator riêng biệt cho mạch analog (ADC/DAC) và mạch số (DSP/MCU) để tránh nhiễu switching

---

## 3. Điều khiển vật lý trên thiết bị (Physical Controls)

|Điều khiển|Loại|Chức năng|
|---|---|---|
|**Bypass Footswitch**|Latching / Momentary footswitch|Chuyển đổi giữa chế độ Bypass (tín hiệu thẳng) và Effect (có xử lý). Hỗ trợ cả **True Bypass** (relay cơ học) và **Buffered Bypass** (chọn qua firmware)|
|**Preset Up / Down**|2 footswitch|Chuyển đổi giữa các preset đã lưu|
|**Tap Tempo**|Footswitch|Nhập BPM bằng chân cho các hiệu ứng time-based (Delay, Tremolo, Vibrato)|
|**Volume Knob**|Potentiometer|Điều chỉnh âm lượng đầu ra tổng thể|
|**Bass / Mid / Treble Knobs**|3x Potentiometer|Điều chỉnh equalizer đầu ra|
|**Expression Pedal Input**|6.35mm TRS jack|Kết nối expression pedal ngoài để điều khiển tham số hiệu ứng real-time (volume, wah, delay mix, v.v.)|
|**LED Indicator**|RGB LED|Màu thể hiện trạng thái: Bypass (trắng), Effect On (màu theo loại effect), Bluetooth (xanh lam), Error (đỏ)|

---

## 4. Effect Engine & Preset System

### 4.1 Kiến trúc Effect Engine

Effect Engine được xây dựng theo mô hình **modular signal graph**: mỗi hiệu ứng là một node độc lập với input/output audio buffer, các node kết nối tuần tự tạo thành effect chain. Engine hỗ trợ:

- **Serial chain:** A → B → C → ... (phổ biến nhất)
- **Parallel routing:** Chia tín hiệu sang 2 nhánh rồi mix lại (ví dụ: Clean + Distortion blend)
- **Wet/Dry Mix:** Kiểm soát tỉ lệ pha trộn tín hiệu gốc và tín hiệu đã xử lý cho từng effect node

### 4.2 Thư viện hiệu ứng tích hợp (Built-in Effect Library)

**Dynamics & Filter:**

- Noise Gate (ngưỡng, thời gian tắt)
- Compressor / Sustainer (threshold, ratio, attack, release, knee)
- Wah-Wah (sweep frequency, Q factor — điều khiển bằng expression pedal)
- Auto-Wah / Envelope Filter

**Gain & Distortion:**

- Clean Boost
- Overdrive (gain, tone, level — mô phỏng Tube Screamer, BluesBreaker)
- Distortion (gain, low/high cut, level — mô phỏng DS-1, Big Muff)
- Fuzz
- Amp Simulation (mô phỏng đặc tính khuếch đại ống: Vox AC30, Fender Twin, Marshall JCM800)

**Modulation:**

- Chorus (rate, depth, mix)
- Flanger (rate, depth, feedback, mix)
- Phaser (rate, depth, feedback, stages)
- Tremolo (rate, depth, waveform: sine/square/triangle)
- Vibrato (rate, depth)
- Pitch Shifter / Harmony (semitones, harmony interval)
- Octaver (sub-octave, up-octave mix)

**Time-based:**

- Delay (time: ms hoặc BPM-sync, feedback, mix; mode: mono/stereo/ping-pong/tape)
- Reverb (mode: room/hall/plate/spring/shimmer; size, decay, pre-delay, mix)
- Echo
- Looper (thu tối đa 60 giây, overdub, undo — điều khiển qua footswitch hoặc app)

**Spatial & Special:**

- Cabinet Simulation / Impulse Response (IR) Loader (hỗ trợ file WAV IR do người dùng nạp vào)
- Ring Modulator
- Bit Crusher / Lo-Fi

### 4.3 Cấu trúc Effect Preset (JSON Schema)

Mỗi preset được lưu trữ dưới dạng file `.json` với cấu trúc chuẩn hóa:

```json
{
  "preset_id": "uuid-v4",
  "name": "Classic Rock Lead",
  "version": "1.0",
  "author": "username",
  "tags": ["rock", "lead", "vintage"],
  "bpm": 120,
  "routing": "serial",
  "effect_chain": [
    {
      "id": "noise_gate",
      "type": "dynamics.noise_gate",
      "enabled": true,
      "params": {
        "threshold_db": -60,
        "release_ms": 100
      }
    },
    {
      "id": "overdrive_1",
      "type": "gain.overdrive",
      "enabled": true,
      "params": {
        "gain": 0.75,
        "tone": 0.6,
        "level": 0.8
      }
    },
    {
      "id": "reverb_1",
      "type": "time.reverb",
      "enabled": true,
      "params": {
        "mode": "hall",
        "size": 0.5,
        "decay": 0.6,
        "pre_delay_ms": 20,
        "mix": 0.25
      }
    }
  ],
  "expression_pedal_target": {
    "effect_id": "overdrive_1",
    "param": "gain",
    "range": [0.0, 1.0]
  },
  "output_eq": {
    "bass_db": 2.0,
    "mid_db": -1.0,
    "treble_db": 3.0
  },
  "created_at": "2026-02-27T10:00:00Z",
  "modified_at": "2026-02-27T10:00:00Z"
}
```

### 4.4 Lưu trữ Preset trên thiết bị

- **Bộ nhớ:** Flash nội bộ hoặc thẻ MicroSD
- **Dung lượng lưu trữ tối thiểu:** 64MB (đủ cho hàng nghìn preset JSON + IR files)
- **Số preset tối đa:** Không giới hạn cứng (phụ thuộc bộ nhớ)
- **Preset mặc định:** 20–30 preset factory được nạp sẵn theo thể loại nhạc
- **Preset slot hoạt động:** Chỉ 1 preset được load vào bộ nhớ DSP tại một thời điểm

---

## 5. Ứng dụng Smartphone (Mobile Application)

### 5.1 Nền tảng

- iOS (Swift / SwiftUI) và Android (Kotlin / Jetpack Compose)
- Hoặc cross-platform: **Flutter** / **React Native**

### 5.2 Tính năng chính

**Quản lý Preset:**

- Giao diện kéo-thả (drag & drop) để thêm, sắp xếp lại, bật/tắt effect node trong chain
- Chỉnh sửa tham số từng hiệu ứng bằng slider, knob ảo, hoặc số
- Nghe preview hiệu ứng qua microphone smartphone (chế độ thử nghiệm)
- Import/Export preset dưới dạng file `.json`
- Tổ chức preset theo thư mục, tag, thể loại nhạc

**Kết nối & Đồng bộ:**

- Ghép đôi với effect box qua BLE
- Đồng bộ hai chiều: đẩy preset từ app lên thiết bị, hoặc kéo preset từ thiết bị về app
- Cập nhật firmware OTA (Over-The-Air) cho effect box
- Remote control: Thay đổi tham số hiệu ứng real-time trong khi chơi (biến smartphone thành controller phụ)

**Tuner tích hợp:**

- Chromatic tuner hiển thị ngay trên app, nhận tín hiệu qua BLE stream từ thiết bị

**Community & Cloud (tính năng nâng cao):**

- Tài khoản người dùng và kho preset trực tuyến (cloud library)
- Chia sẻ preset với cộng đồng, đánh giá và download preset của người khác
- Tích hợp chia sẻ qua QR Code (scan để import preset ngay lập tức)

---

## 6. Tính năng đặc biệt (Advanced Features)

### 6.1 Built-in Chromatic Tuner

- Thuật toán phát hiện pitch: autocorrelation hoặc YIN algorithm
- Độ chính xác: ±1 cent
- Hiển thị trên màn hình OLED của thiết bị và trên app
- **Tuner mode:** Tắt tiếng đầu ra (mute output) trong khi lên dây

### 6.2 Looper

- Thời gian thu: tối thiểu 60 giây (phụ thuộc RAM DSP)
- Chức năng: Record → Playback → Overdub → Undo/Redo → Clear
- Điều khiển qua footswitch chuyên dụng hoặc ứng dụng smartphone
- Đồng bộ BPM với Tap Tempo

### 6.3 IR (Impulse Response) Loader

- Hỗ trợ nạp file IR cabinet simulation (.wav, mono, 44.1/48kHz)
- Người dùng có thể nạp IR của các thương hiệu loa nổi tiếng (Celestion, Mesa/Boogie, v.v.)
- IR được nhúng trực tiếp vào preset JSON (base64 encode) hoặc lưu riêng với ID tham chiếu

### 6.4 Tap Tempo & BPM Sync

- Nhấn footswitch liên tiếp để xác định BPM
- Tự động sync tốc độ của Delay, Tremolo, Chorus, Phaser, Vibrato theo BPM
- Hỗ trợ các chia nhỏ nhịp: 1/4, 1/8, dotted 1/8, 1/16

### 6.5 MIDI Control

- Nhận MIDI Program Change để chuyển preset
- Nhận MIDI Control Change (CC) để điều khiển tham số hiệu ứng
- Nhận MIDI Clock để đồng bộ BPM
- Gửi MIDI Thru để kết nối chuỗi nhiều thiết bị MIDI

### 6.6 USB Audio Interface Mode

- Khi kết nối USB-C với máy tính, thiết bị hoạt động như một **USB Audio Interface 24-bit/48kHz** (USB Audio Class 2.0, không cần driver)
- Cho phép thu âm trực tiếp vào DAW với hiệu ứng đã xử lý hoặc tín hiệu clean (re-amping)

---

## 7. Thông số kỹ thuật tổng hợp (Summary Specifications)

|Thông số|Giá trị|
|---|---|
|Đầu vào nhạc cụ|1x 6.35mm TS (Hi-Z, ≥ 1MΩ)|
|Đầu ra line|1x 6.35mm TRS (unbalanced)|
|Đầu ra DI|1x XLR (balanced, ground lift)|
|Expression Pedal|1x 6.35mm TRS|
|MIDI|TRS Type-A (In/Out) hoặc DIN 5-pin|
|USB|USB Type-C (Audio Class 2.0 + Power + Data)|
|Bluetooth|BLE 5.0|
|Sample Rate / Bit Depth|48 kHz / 24-bit|
|End-to-end Latency|< 10ms (mục tiêu < 5ms)|
|Dynamic Range|> 100 dB (A-weighted)|
|THD+N|< 0.01%|
|Nguồn|9V DC center-negative, 500mA (tiêu chuẩn pedalboard)|
|Pin tùy chọn|Li-Po 3000mAh, ≥ 6 giờ|
|Preset lưu trữ|MicroSD, không giới hạn|
|Kích thước|~120 × 80 × 55 mm (compact pedal form factor)|
|Vỏ|Nhôm đúc (die-cast aluminum)|

---

## 8. Lộ trình phát triển (Development Roadmap)

|Giai đoạn|Mục tiêu|Thời gian (ước tính)|
|---|---|---|
|**Phase 1 – Prototype**|Hardware prototype (ADC/DAC/DSP trên Raspberry Pi), firmware cơ bản, 5 effect cốt lõi, app MVP|3–4 tháng|
|**Phase 2 – Alpha**|Chuyển sang hardware mục tiêu (Bela/STM32), đủ bộ effect library, BLE hoàn chỉnh, app beta|3–4 tháng|
|**Phase 3 – Beta**|Tối ưu latency, kiểm thử âm thanh, thiết kế PCB, vỏ hộp, MIDI, Looper, Tuner|2–3 tháng|
|**Phase 4 – Release**|PCB sản xuất, firmware ổn định, app phát hành, tài liệu SDK mở cho cộng đồng|2–3 tháng|
