// ============================================
// PROJECT MONITORING SUHU, KELEMBAPAN & GAS
// ESP32 + DHT11 + MQ135 + LCD I2C + SD CARD
// ============================================
// Library yang dibutuhkan (install via Library Manager):
//   - DHT sensor library (by Adafruit)
//   - LiquidCrystal I2C (by Frank de Brabander)
//   - ArduinoJson (by Benoit Blanchon)
//   - SD (sudah bawaan Arduino IDE)
// ============================================

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <time.h>

// =============================================
// --- KONFIGURASI (SESUAIKAN DI SINI) ---
// =============================================

// WiFi
const char* ssid     = "NAMA_WIFI_KAMU";
const char* password = "PASSWORD_WIFI_KAMU";

// Server (ganti dengan URL Railway/Render kamu setelah deploy)
const char* serverURL = "https://nama-proyekmu.up.railway.app/api/data";

// Pin
#define DHT_PIN     4     // Pin data DHT11
#define DHT_TYPE    DHT11
#define MQ135_PIN   34    // Pin analog MQ135 (hanya baca analog)
#define SD_CS_PIN   5     // Chip Select kartu SD

// Batas alarm
const float BATAS_SUHU_MAX = 35.0;   // Celsius
const float BATAS_SUHU_MIN = 10.0;   // Celsius
const int   BATAS_GAS_MAX  = 400;    // Nilai ADC (0-4095)

// =============================================
// OBJEK SENSOR & LCD
// =============================================
DHT dht(DHT_PIN, DHT_TYPE);

// Alamat I2C LCD: coba 0x27 dulu, jika tidak muncul ganti 0x3F
LiquidCrystal_I2C lcd(0x27, 20, 4);

// =============================================
// VARIABEL GLOBAL
// =============================================
int  intervalDetik = 5;    // Interval pengiriman data (default 5 detik)
bool sedangJalan   = true; // Status start/stop

// =============================================
void setup() {
  Serial.begin(115200);
  Serial.println("=== Sistem Monitoring Sensor ===");

  // --- Inisialisasi DHT11 ---
  dht.begin();
  Serial.println("DHT11 siap.");

  // --- Inisialisasi LCD ---
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Monitoring");
  lcd.setCursor(0, 1);
  lcd.print("Menghubungkan WiFi..");
  Serial.println("LCD siap.");

  // --- Inisialisasi Kartu SD ---
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[SD] Kartu SD gagal diinisialisasi!");
    lcd.setCursor(0, 2);
    lcd.print("SD GAGAL!");
  } else {
    Serial.println("[SD] Kartu SD OK.");
    // Buat header file CSV jika belum ada
    if (!SD.exists("/data.csv")) {
      File f = SD.open("/data.csv", FILE_WRITE);
      if (f) {
        f.println("Waktu,Suhu(C),Kelembapan(%),Gas(ADC),Status");
        f.close();
        Serial.println("[SD] File data.csv dibuat.");
      }
    }
  }

  // --- Koneksi WiFi ---
  Serial.print("[WiFi] Menghubungkan ke ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  int percobaan = 0;
  while (WiFi.status() != WL_CONNECTED && percobaan < 20) {
    delay(500);
    Serial.print(".");
    percobaan++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Terhubung! IP: " + WiFi.localIP().toString());
    lcd.setCursor(0, 2);
    lcd.print("WiFi: Terhubung     ");
    lcd.setCursor(0, 3);
    lcd.print(WiFi.localIP().toString());
    // Sinkronisasi waktu NTP (WIB = UTC+7)
    configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.println("[NTP] Sinkronisasi waktu...");
    delay(2000);
  } else {
    Serial.println("\n[WiFi] Gagal konek! Mode offline.");
    lcd.setCursor(0, 2);
    lcd.print("WiFi: GAGAL (offline)");
  }

  delay(2000);
  lcd.clear();
  Serial.println("Setup selesai. Memulai monitoring...");
}

// =============================================
void loop() {
  // Jika sedang dihentikan (stop dari website)
  if (!sedangJalan) {
    tampilkanLCDStop();
    delay(1000);
    cekPerintahServer();
    return;
  }

  // --- Baca sensor DHT11 ---
  float suhu       = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  // Validasi data DHT11
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("[DHT] Gagal membaca sensor! Periksa kabel.");
    suhu       = 0.0;
    kelembapan = 0.0;
  }

  // --- Baca sensor MQ135 ---
  int nilaiGas = analogRead(MQ135_PIN);

  // --- Tentukan status alarm ---
  bool   adaAlarm   = false;
  String statusAlarm = "Normal";

  if (suhu > BATAS_SUHU_MAX) {
    adaAlarm   = true;
    statusAlarm = "Suhu Terlalu Tinggi!";
  } else if (suhu < BATAS_SUHU_MIN && suhu > 0) {
    adaAlarm   = true;
    statusAlarm = "Suhu Terlalu Rendah!";
  }
  if (nilaiGas > BATAS_GAS_MAX) {
    adaAlarm   = true;
    statusAlarm = "Gas Berbahaya!";
  }

  // Log ke Serial
  Serial.println("---");
  Serial.println("Suhu       : " + String(suhu)       + " C");
  Serial.println("Kelembapan : " + String(kelembapan)  + " %");
  Serial.println("Gas MQ135  : " + String(nilaiGas)    + " ADC");
  Serial.println("Status     : " + statusAlarm);

  // Dapatkan waktu
  String waktu = dapatkanWaktu();

  // --- Tampilkan di LCD ---
  tampilkanLCD(suhu, kelembapan, nilaiGas, statusAlarm);

  // --- Simpan ke Kartu SD ---
  simpanKeSD(waktu, suhu, kelembapan, nilaiGas, statusAlarm);

  // --- Kirim ke Server ---
  kirimKeServer(waktu, suhu, kelembapan, nilaiGas, statusAlarm);

  // --- Cek Perintah dari Server (interval & start/stop) ---
  cekPerintahServer();

  // Tunggu sesuai interval
  delay(intervalDetik * 1000);
}

// =============================================
// FUNGSI: Tampilkan data di LCD
// =============================================
void tampilkanLCD(float suhu, float kelembapan, int gas, String status) {
  lcd.clear();

  // Baris 1: Suhu
  lcd.setCursor(0, 0);
  lcd.print("Suhu : ");
  lcd.print(suhu, 1);
  lcd.print(" C");

  // Baris 2: Kelembapan
  lcd.setCursor(0, 1);
  lcd.print("RH   : ");
  lcd.print(kelembapan, 1);
  lcd.print(" %");

  // Baris 3: Gas MQ135
  lcd.setCursor(0, 2);
  lcd.print("Gas  : ");
  lcd.print(gas);
  lcd.print(" ADC");

  // Baris 4: Status
  lcd.setCursor(0, 3);
  if (status == "Normal") {
    lcd.print("Status : Normal     ");
  } else {
    lcd.print("! ");
    // Potong teks jika terlalu panjang (maks 20 karakter)
    if (status.length() > 18) {
      lcd.print(status.substring(0, 18));
    } else {
      lcd.print(status);
    }
  }
}

// Tampilan LCD saat monitoring dihentikan
void tampilkanLCDStop() {
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("--- BERHENTI ---");
  lcd.setCursor(1, 2);
  lcd.print("Monitoring Dihentikan");
}

// =============================================
// FUNGSI: Simpan data ke Kartu SD
// =============================================
void simpanKeSD(String waktu, float suhu, float kelembapan, int gas, String status) {
  File dataFile = SD.open("/data.csv", FILE_APPEND);
  if (dataFile) {
    dataFile.print(waktu);
    dataFile.print(",");
    dataFile.print(suhu, 1);
    dataFile.print(",");
    dataFile.print(kelembapan, 1);
    dataFile.print(",");
    dataFile.print(gas);
    dataFile.print(",");
    dataFile.println(status);
    dataFile.close();
    Serial.println("[SD] Data tersimpan.");
  } else {
    Serial.println("[SD] Gagal membuka file!");
  }
}

// =============================================
// FUNGSI: Kirim data ke server via HTTP POST
// =============================================
void kirimKeServer(String waktu, float suhu, float kelembapan, int gas, String status) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] Tidak ada koneksi WiFi, skip kirim.");
    return;
  }

  HTTPClient http;
  http.begin(serverURL);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(5000); // Timeout 5 detik

  // Buat payload JSON
  StaticJsonDocument<256> doc;
  doc["waktu"]      = waktu;
  doc["suhu"]       = suhu;
  doc["kelembapan"] = kelembapan;
  doc["gas"]        = gas;
  doc["status"]     = status;

  String jsonData;
  serializeJson(doc, jsonData);

  int kodeHttp = http.POST(jsonData);

  if (kodeHttp > 0) {
    Serial.println("[HTTP] Terkirim. Kode: " + String(kodeHttp));
  } else {
    Serial.println("[HTTP] Gagal kirim. Error: " + http.errorToString(kodeHttp));
  }

  http.end();
}

// =============================================
// FUNGSI: Cek perintah dari server (interval & start/stop)
// =============================================
void cekPerintahServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  // Buat URL endpoint perintah
  String baseURL = String(serverURL);
  int pos = baseURL.lastIndexOf("/api/data");
  if (pos < 0) return;
  String urlPerintah = baseURL.substring(0, pos) + "/api/perintah";

  HTTPClient http;
  http.begin(urlPerintah);
  http.setTimeout(3000);
  int kode = http.GET();

  if (kode == 200) {
    String respons = http.getString();
    StaticJsonDocument<128> doc;
    DeserializationError err = deserializeJson(doc, respons);

    if (!err) {
      if (doc.containsKey("interval")) {
        int intervalBaru = doc["interval"];
        if (intervalBaru != intervalDetik) {
          intervalDetik = intervalBaru;
          Serial.println("[CMD] Interval diubah ke: " + String(intervalDetik) + " detik");
        }
      }
      if (doc.containsKey("jalan")) {
        bool jalanBaru = doc["jalan"];
        if (jalanBaru != sedangJalan) {
          sedangJalan = jalanBaru;
          Serial.println("[CMD] Status diubah: " + String(sedangJalan ? "JALAN" : "BERHENTI"));
        }
      }
    }
  }

  http.end();
}

// =============================================
// FUNGSI: Dapatkan waktu dari NTP
// =============================================
String dapatkanWaktu() {
  struct tm infoWaktu;
  if (!getLocalTime(&infoWaktu)) {
    return "Waktu tidak tersedia";
  }
  char buffer[25];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &infoWaktu);
  return String(buffer);
}
