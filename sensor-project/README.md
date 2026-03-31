# 🌡️ PROJECT MONITORING SENSOR ESP32
## Suhu, Kelembapan (DHT11) & Gas (MQ135) + LCD + SD Card + Website Publik

---

## 📦 STRUKTUR FILE

```
sensor-project/
├── esp32_sensor.ino      ← Kode program untuk ESP32 (upload via Arduino IDE)
├── server.js             ← Server Node.js (backend website)
├── package.json          ← Daftar dependensi Node.js
├── README.md             ← Panduan ini
└── public/
    └── index.html        ← Tampilan website (otomatis dilayani server)
```

---

## 🔌 SKEMA WIRING (PIN ESP32)

| Komponen     | Pin ESP32    | Keterangan                        |
|--------------|--------------|-----------------------------------|
| DHT11 VCC    | 3.3V         | Tegangan sensor                   |
| DHT11 DATA   | GPIO 4       | Pin data digital                  |
| DHT11 GND    | GND          | Ground                            |
| MQ135 VCC    | 5V           | Tegangan sensor (butuh 5V)        |
| MQ135 AOUT   | GPIO 34      | Output analog (ADC)               |
| MQ135 GND    | GND          | Ground                            |
| LCD SDA      | GPIO 21      | I2C Data                          |
| LCD SCL      | GPIO 22      | I2C Clock                         |
| LCD VCC      | 5V           | Tegangan LCD                      |
| LCD GND      | GND          | Ground                            |
| SD CS        | GPIO 5       | Chip Select kartu SD              |
| SD SCK       | GPIO 18      | SPI Clock                         |
| SD MOSI      | GPIO 23      | SPI Data out                      |
| SD MISO      | GPIO 19      | SPI Data in                       |
| SD VCC       | 3.3V         | Tegangan modul SD                 |
| SD GND       | GND          | Ground                            |

> ⚠️ PENTING: Gunakan modul LCD dengan adapter I2C agar hemat pin dan kabel.
> Alamat I2C default: 0x27. Jika LCD tidak menyala, coba ganti ke 0x3F di baris lcd(0x27...) kode ESP32.

---

## 🛠️ LANGKAH PENGGUNAAN

### STEP 1 — Siapkan Arduino IDE

1. Install Arduino IDE dari https://arduino.cc
2. Tambahkan board ESP32:
   - File → Preferences → Additional Board Manager URLs:
     `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Board Manager → cari "esp32" → Install

3. Install library (Sketch → Include Library → Manage Libraries):
   - `DHT sensor library` (by Adafruit)
   - `LiquidCrystal I2C` (by Frank de Brabander)
   - `ArduinoJson` (by Benoit Blanchon)
   - `SD` (sudah bawaan)

---

### STEP 2 — Deploy Server ke Internet (Railway - GRATIS)

1. Buat akun di https://github.com
2. Buat akun di https://railway.app (login pakai GitHub)
3. Upload folder ini ke GitHub:
   ```bash
   git init
   git add .
   git commit -m "sensor monitoring project"
   ```
   Buat repo baru di GitHub, ikuti instruksi push yang diberikan.

4. Di Railway:
   - Klik "New Project" → "Deploy from GitHub repo"
   - Pilih repo yang baru dibuat
   - Tunggu 1-2 menit hingga muncul URL publik

5. Salin URL dari Railway (contoh: `https://sensor-abc123.up.railway.app`)

---

### STEP 3 — Konfigurasi & Upload Kode ESP32

1. Buka file `esp32_sensor.ino` di Arduino IDE
2. Ubah baris berikut sesuai kondisi kamu:
   ```cpp
   const char* ssid     = "NAMA_WIFI_KAMU";
   const char* password = "PASSWORD_WIFI_KAMU";
   const char* serverURL = "https://URL-RAILWAY-KAMU.up.railway.app/api/data";
   ```
3. Pilih board: Tools → Board → ESP32 Dev Module
4. Pilih port: Tools → Port → (pilih port COM ESP32)
5. Klik Upload (panah ke kanan)
6. Buka Serial Monitor (baud 115200) untuk melihat log

---

### STEP 4 — Akses Website

Buka URL Railway di browser (contoh: `https://sensor-abc123.up.railway.app`).
Website dapat diakses dari mana saja tanpa login (open access).

---

## 🌐 FITUR WEBSITE

| Fitur                | Keterangan                                              |
|----------------------|---------------------------------------------------------|
| 📊 Kartu real-time   | Nilai suhu, kelembapan, gas, dan status terkini         |
| 📈 Grafik            | Line chart 50 data terakhir, auto-update tiap 3 detik   |
| 📋 Tabel             | 30 data terakhir, terbaru di atas                       |
| ⚠️ Alarm             | Banner merah dan kartu status berubah jika tidak normal |
| ▶ Start / ⏹ Stop    | Kontrol monitoring dari website, ESP32 ikut             |
| ⏱ Set Interval       | Ubah frekuensi pengiriman data (1-3600 detik)           |
| ⬇️ Export CSV        | Unduh semua data dalam format CSV (bisa dibuka Excel)   |
| 🗑 Hapus Data         | Hapus semua data dari server (SD card tidak terpengaruh)|

---

## 📡 ENDPOINT API SERVER

| Method | URL               | Fungsi                                    |
|--------|-------------------|-------------------------------------------|
| POST   | /api/data         | Terima data dari ESP32                    |
| GET    | /api/data         | Ambil semua data (untuk website)          |
| GET    | /api/terbaru      | Ambil 1 data terbaru                      |
| GET    | /api/perintah     | Ambil pengaturan (dipanggil ESP32)        |
| POST   | /api/pengaturan   | Ubah interval/status dari website         |
| GET    | /api/export       | Unduh data sebagai file CSV               |
| DELETE | /api/data         | Hapus semua data                          |

---

## ⚠️ BATAS ALARM (Bisa Diubah)

Di file `esp32_sensor.ino`:
```cpp
const float BATAS_SUHU_MAX = 35.0;  // Celsius
const float BATAS_SUHU_MIN = 10.0;  // Celsius
const int   BATAS_GAS_MAX  = 400;   // ADC (0-4095)
```

---

## 🛡️ CATATAN PENTING

- Data server tersimpan di **memori** (hilang saat server restart).
  Untuk penyimpanan permanen, gunakan MongoDB Atlas (gratis).
- Data di **kartu SD** tersimpan permanen di file `/data.csv`.
- Kartu SD harus diformat **FAT32** (maksimal 32GB).
- ESP32 cek perintah dari server setiap selesai satu interval pengukuran.
- Waktu ditampilkan dalam **WIB (UTC+7)**. Ganti angka 7 di `configTime(7 * 3600...)` jika berbeda zona waktu.

---

## 🧪 URUTAN PENGUJIAN

1. ✅ Pastikan server Railway aktif (buka URL di browser → harus tampil website)
2. ✅ Buka Serial Monitor ESP32, pastikan muncul "WiFi terhubung" dan "Data terkirim"
3. ✅ Buka website, data harus muncul dalam beberapa detik
4. ✅ Uji tombol Stop → cek Serial Monitor ESP32 berhenti kirim data
5. ✅ Uji Export CSV → file harus terunduh

---

Dibuat untuk keperluan project monitoring lingkungan dengan ESP32.
