# Electrical Sequencer System

Sistem embedded untuk mengontrol 8 channel outlet 220VAC secara berurutan (sequencing) dengan proteksi Overvoltage, Undervoltage, dan Overcurrent. Dikontrol penuh secara lokal via LCD I2C 20x4 dan 4 tombol push button — tanpa IoT.

## Daftar Isi

- [Gambaran Umum](#gambaran-umum)
- [Spesifikasi Hardware](#spesifikasi-hardware)
  - [Controller](#controller)
  - [Input](#input)
  - [Output](#output)
  - [Display](#display)
- [Spesifikasi Software](#spesifikasi-software)
  - [IDE & Board](#ide--board)
  - [Library](#library)
- [Wiring / Koneksi](#wiring--koneksi)
  - [PZEM-004T (UART)](#pzem-004t-uart)
  - [ADS1115 (I2C)](#ads1115-i2c)
  - [8 Channel Relay](#8-channel-relay)
  - [Push Button](#push-button)
- [Fitur Sistem](#fitur-sistem)
  - [Monitoring](#monitoring)
  - [Sequence ON/OFF](#sequence-onoff)
  - [Configuration Menu](#configuration-menu)
  - [Protection](#protection)
- [Custom Byte Char LCD](#custom-byte-char-lcd)
- [State Machine](#state-machine)
- [Self-Healing & Optimasi Memori](#self-healing--optimasi-memori)
- [Struktur File](#struktur-file)
- [Konfigurasi](#konfigurasi)
  - [Hardcoded Config (config.h)](#hardcoded-config-configh)
  - [User Config (LCD Menu - NVS)](#user-config-lcd-menu---nvs)
- [Cara Compile & Upload](#cara-compile--upload)
- [Kredit](#kredit)

---

## Gambaran Umum

Alat ini berfungsi untuk menyalakan 8 output outlet 220VAC secara berurutan (sequencing) melalui 8 channel relay yang dikontrol oleh **ESP32 Devkit V1**. Dilengkapi dengan:

- **Proteksi Master Source** (Under/Overvoltage & Overcurrent via PZEM-004T)
- **Proteksi Overcurrent per-channel** (via ACS712 + ADS1115)
- **Monitoring real-time** tegangan, arus, frekuensi, daya
- **Konfigurasi penuh** via LCD 20x4 + 4 push button
- **Sequencing otomatis** dengan interval per-channel yang dapat diatur

Target user: menengah-awam dengan mengedepankan user-friendly.

---

## Spesifikasi Hardware

### Controller

| Item | Spesifikasi |
|------|-------------|
| **MCU** | ESP32 Devkit V1 (30 pin) |
| **Clock** | 240 MHz |
| **Flash** | 4 MB |
| **SRAM** | 520 KB |

### Input

| Sensor | Interface | Fungsi |
|--------|-----------|--------|
| **1x PZEM-004T 100A** | UART (RX2/TX2) | Monitoring tegangan, arus, frekuensi, energi master source |
| **8x ACS712** | Analog (via ADS1115) | Sensor arus per-channel outlet AC |
| **2x ADS1115** | I2C (addr 0x48 & 0x49) | ADC 16-bit external untuk 8 channel ACS712 (masing-masing handle 4 channel) |
| **4x Push Button NO** | GPIO (internal pullup) | 1x SQC_BTN (sequence), 3x Menu (LEFT, CENTER, RIGHT) |

### Output

| Item | Spesifikasi |
|------|-------------|
| **8 Channel Relay** | Active Low 5V Optocoupler, Normally OFF saat booting |
| **GPIO** | CH1=13, CH2=18, CH3=19, CH4=23, CH5=25, CH6=26, CH7=27, CH8=32 |

### Display

| Item | Spesifikasi |
|------|-------------|
| **LCD** | 20 kolom x 4 baris, I2C (addr 0x27) |
| **Library** | LiquidCrystal_I2C (custom, kompatibel ESP32) |

---

## Spesifikasi Software

### IDE & Board

| Tool | Versi |
|------|-------|
| Arduino IDE | v2.3.6 |
| Board Package | esp32 v3.3.0 (Board: ESP32 Dev Module) |

### Library

| Library | Versi | Keterangan |
|---------|-------|------------|
| LiquidCrystal_I2C | (custom) | Library LCD milik sendiri, kompatibel ESP32 |
| PZEM004Tv30 | v1.2.1 | Oleh Jakub Mandula |
| Adafruit ADS1X15 | v2.6.2 | Oleh Adafruit |

---

## Wiring / Koneksi

### PZEM-004T (UART)

| PZEM | ESP32 |
|------|-------|
| RX | GPIO 17 (TX2) |
| TX | GPIO 16 (RX2) |

### ADS1115 (I2C)

| ADS1115 #1 (0x48) | ADS1115 #2 (0x49) |
|--------------------|--------------------|
| CH1-CH4 ACS712 | CH5-CH8 ACS712 |

| Signal | ESP32 |
|--------|-------|
| SDA | GPIO 21 |
| SCL | GPIO 22 |

### 8 Channel Relay

| Relay | GPIO | Keterangan |
|-------|------|------------|
| CH 1 | 13 | Active Low |
| CH 2 | 18 | Active Low |
| CH 3 | 19 | Active Low |
| CH 4 | 23 | Active Low |
| CH 5 | 25 | Active Low |
| CH 6 | 26 | Active Low |
| CH 7 | 27 | Active Low |
| CH 8 | 32 | Active Low |

### Push Button

| Tombol | GPIO | Internal Pullup |
|--------|------|-----------------|
| SQC_BTN | 34 | Ya |
| LEFT_BTN | 35 | Ya |
| CENTER_BTN | 36 | Ya |
| RIGHT_BTN | 39 | Ya |

---

## Fitur Sistem

### Monitoring

Terdapat 3 scene monitoring yang dapat di-scroll dengan LEFT/RIGHT secara **endless**:

| Scene | Tampilan | Keterangan |
|-------|----------|------------|
| **2a** | Master Source (PZEM) | Tegangan, arus, frekuensi, daya + indikator channel 1-8 di baris 4. Channel TRIP berkedip. |
| **2b** | Channel Current (ACS712) | Arus per-channel CH1-CH8 dalam Ampere |
| **2c** | Channel Status | Status ON/OFF/TRIP per-channel. Bisa masuk ke write mode untuk reset TRIP atau kontrol ON/OFF manual. |

**Write Mode (Scene 2c):**
- CENTER short → masuk write mode
- LEFT/RIGHT → pilih channel
- CENTER short → pilih ON/OFF
- CENTER short → eksekusi
- CENTER long → kembali read-only

### Sequence ON/OFF

**SQC_BTN** (hold 2 detik, configurable):

1. Cek tegangan master source (PZEM) — harus dalam range normal (OV > V > UV)
2. Jika range normal & semua channel OFF → **Sequence ON**: nyalakan channel berurutan sesuai interval (channel dengan limit 0.0A di-skip)
3. Jika range normal & semua channel ON → **Sequence OFF**: matikan channel berurutan sesuai interval
4. Jika tegangan di luar range → tampilkan alert, tolak sequence
5. Jika sedang dalam proses sequence → input diabaikan (debounce)

### Configuration Menu

Menu navigasi **endless** dengan LEFT/RIGHT, CENTER short untuk enter/konfirmasi, CENTER long untuk back.

| Menu | Fungsi |
|------|--------|
| **Set Master Source** | Atur OV, UV, I Limit threshold untuk PZEM |
| **Set Limit Arus CH** | Atur limit arus per-channel (total ≤ I Limit PZEM, channel 0.0A di-skip sequence) |
| **Set Interval SQC** | Atur interval waktu antar channel saat sequence (INV1-INV8) |
| **Set Calibration** | Enable/disable auto-zero calibration saat booting |

Setiap selesai menyimpan setting → tampilkan `"Saved!"` 1 detik.

### Protection

| Level | Sensor | Trigger | Action (config.h) |
|-------|--------|---------|-------------------|
| **Master OV** | PZEM | V ≥ OV threshold | TRIP ALL + Alert atau Alert Only |
| **Master UV** | PZEM | V ≤ UV threshold | TRIP ALL + Alert atau Alert Only |
| **Master I Limit** | PZEM | I ≥ I Limit threshold | TRIP ALL + Alert atau Alert Only |
| **Per-channel OC** | ACS712 | I channel ≥ limit CH | TRIP channel tersebut |

**Trip Recovery:** Reset via Scene 2c write mode.

---

## Custom Byte Char LCD

Custom character untuk indikator channel 1-8 pada baris ke-4 Scene 2a. Disimpan di CGRAM posisi 0-7.

| CGRAM Pos | Normal | Inverse (TRIP) |
|-----------|--------|----------------|
| 0 | ch1 | ch1inv |
| 1 | ch2 | ch2inv |
| 2 | ch3 | ch3inv |
| 3 | ch4 | ch4inv |
| 4 | ch5 | ch5inv |
| 5 | ch6 | ch6inv |
| 6 | ch7 | ch7inv |
| 7 | ch8 | ch8inv |

Saat channel TRIP, karakter akan bergantian antara normal dan inverse dengan interval kedipan yang dapat diatur di `config.h` (default: 500ms).

Sumber data byte: `indicator_multiplexer_test.ino`.

---

## State Machine

Sistem berjalan secara asynchronous (non-blocking) tanpa `delay()`:

```
IDLE → OPENING → CALIBRATION (if enabled) → MONITORING ↔ CONFIG
```

| State | Deskripsi |
|-------|-----------|
| **STATE_IDLE** | Inisialisasi hardware saat boot |
| **STATE_OPENING** | Tampilkan opening scene 1 & 2 |
| **STATE_CALIBRATION** | Auto-zero calibration ACS712 (jika enabled) |
| **STATE_MONITORING** | Main loop: display scene 2a/2b/2c, handle SQC_BTN |
| **STATE_CONFIG** | Menu konfigurasi (enter dari monitoring, long press CENTER back) |

Semua state menggunakan `millis()` timer.

---

## Self-Healing & Optimasi Memori

### Memory Management
- Dilarang menggunakan `String` class, `malloc`/`free`, `new`/`delete`
- Semua buffer static/fixed-size (`char[]`)
- Gunakan `snprintf`, bukan `sprintf`

### Heap Monitoring
- `heap_caps_get_free_size(MALLOC_CAP_INTERNAL)` periodik (tiap 10 menit)
- Jika free heap < 20% dari total awal → restart otomatis

### Task Watchdog
- TWDT aktif untuk deteksi hang
- Setiap state machine loop feed watchdog

### I2C Bus Recovery
- Jika I2C gagal → reset bus, reconnect
- 3x gagal → tampilkan error screen

### UART/PZEM Recovery
- Jika PZEM tidak merespon → restart UART, re-init
- 3x gagal → tampilkan "PZEM Error!"

### Auto-Restart Terjadwal (opsional)
- Enable/disable di `config.h`
- Restart tiap N jam saat semua channel OFF

---

## Struktur File

```
electric_outlet_sequencer/
├── electric_outlet_sequencer.ino   # Main program
├── config.h                        # Hardcoded configuration
├── data_acquisition.h/.cpp         # Baca sensor PZEM & ACS712
├── calibration.h/.cpp              # Auto-zero calibration ACS712
├── display.h/.cpp                  # Manajemen tampilan LCD
├── LiquidCrystal_I2C.h/.cpp        # Library LCD (custom)
├── menu.h/.cpp                     # Menu konfigurasi LCD
├── byte_char.h                     # Custom byte char untuk channel indicator
├── progress.txt                    # Log progress pengerjaan
├── brief.txt                       # Dokumen brief project
└── README.md                       # Dokumentasi ini
```

Seluruh kode menggunakan **Doxygen style** dengan Bahasa Indonesia untuk dokumentasi fungsi dan baris kode.

---

## Konfigurasi

### Hardcoded Config (config.h)

Konfigurasi yang hanya bisa diubah oleh programmer:

| Parameter | Deskripsi |
|-----------|-----------|
| `OV_ACTION` | TRIP_ALL atau ALERT_ONLY saat overvoltage |
| `UV_ACTION` | TRIP_ALL atau ALERT_ONLY saat undervoltage |
| `ILIMIT_ACTION` | TRIP_ALL atau ALERT_ONLY saat overcurrent PZEM |
| `BLINK_INTERVAL_MS` | Interval kedipan indikator TRIP (default: 500ms) |
| `SQC_BTN_DELAY_MS` | Hold time SQC_BTN (default: 2000ms) |
| `LONG_PRESS_MS` | Durasi long press CENTER (default: 1000ms) |
| `OPENING_SCENE_2_ENABLE` | Enable/disable opening scene 2 |
| `OPENING_SCENE_DURATION_MS` | Durasi masing-masing opening scene |
| `AUTO_RESTART_ENABLE` | Enable/disable auto-restart terjadwal |
| `AUTO_RESTART_INTERVAL_H` | Interval auto-restart (jam) |

### User Config (LCD Menu - NVS)

Disimpan di NVS (Preferences) ESP32:

| Key (NVS) | Tipe | Deskripsi |
|-----------|------|-----------|
| `ov_thresh` | float | Threshold Overvoltage (V) |
| `uv_thresh` | float | Threshold Undervoltage (V) |
| `i_limit` | float | Limit arus PZEM (A) |
| `ch_lim_0` - `ch_lim_7` | float | Limit arus per-channel (A) |
| `sqc_int_0` - `sqc_int_7` | uint16_t | Interval sequence per-channel (ms) |
| `auto_cal` | bool | Auto calibration ON/OFF |
| `sqc_btn_dly` | uint16_t | SQC button hold time (ms) |
| `blink_int` | uint16_t | Trip blink interval (ms) |
| `op_scene2` | bool | Opening scene 2 ON/OFF |

Default value dimuat dari `config.h` saat first boot (NVS kosong / CRC mismatch).

---

## Cara Compile & Upload

1. Buka `electric_outlet_sequencer.ino` di **Arduino IDE v2.3.6**
2. Install **ESP32 Board Package v3.3.0**: Tools → Board → Boards Manager → cari "ESP32"
3. Install library yang diperlukan:
   - PZEM004Tv30 (oleh Jakub Mandula)
   - Adafruit ADS1X15 (oleh Adafruit)
4. Copy folder `LiquidCrystal_I2C` ke folder `libraries` Arduino
5. Pilih board: **Tools → Board → ESP32 Dev Module**
6. Pilih port yang sesuai
7. Klik **Upload**

---

## Kredit

- **Politeknik Negeri Sriwijaya (POLSRI)**
- **M. Wahyu Padhroni** — 062330320656
- **Ir. M. Nawawi, M.T.** — Pembimbing 1
- **Ir. Iskandar Lutfi, M.T.** — Pembimbing 2
