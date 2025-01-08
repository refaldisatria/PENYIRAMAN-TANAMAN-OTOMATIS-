#define BLYNK_TEMPLATE_ID "TMPL67eFbCFfQ"
#define BLYNK_TEMPLATE_NAME "PENYIRAMAN TANAMAN"
#define BLYNK_AUTH_TOKEN "-0LVA82qaXKe1y86FZrTESENd9vGWtya"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// Informasi WiFi
char ssid[] = "Hp Cino";
char pass[] = "andasiapa";

// Konfigurasi LCD
LiquidCrystal_PCF8574 lcd(0x27);

// Definisi pin
#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D1
#define RELAY_FERTILIZER_PIN D6
#define SDA_PIN D2
#define SCL_PIN D5

// Variabel sistem
int soilMoistureValue;
int thresholdDry = 600;
bool pumpState = false;          // Status pompa air
bool fertilizerPumpState = false; // Status pompa pupuk
bool manualMode = false;         // Mode manual
int waterPumpInterval = 4;       // Interval penyiraman (jam)
int waterPumpDuration = 30;      // Durasi pompa air (detik)
int fertilizerPumpDuration = 10; // Durasi pompa pupuk (detik)
unsigned long lastWaterPumpTime = 0;
unsigned long lastFertilizerPumpTime = 0;
unsigned long lastSoilMoistureCheck = 0;
unsigned long autoWaterLockTime = 0;
int autoWaterFrequency = 3;      // Frekuensi penyiraman otomatis per hari
unsigned long autoWaterInterval = 86400000 / autoWaterFrequency; // Interval penyiraman otomatis dalam milidetik

// Timer
BlynkTimer timer;

void setup() {
  Serial.begin(115200);

  // Inisialisasi I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inisialisasi pin
  pinMode(SOIL_MOISTURE_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_FERTILIZER_PIN, OUTPUT);

  // Matikan pompa awalnya
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RELAY_FERTILIZER_PIN, HIGH);

  // Inisialisasi LCD
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Menghubungkan...");
  lcd.setCursor(0, 1);
  lcd.print("WiFi & Blynk...");

  // Hubungkan ke WiFi dan Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Tampilkan pesan sistem aktif
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem Aktif");

  // Atur timer untuk membaca kelembapan tanah
  timer.setInterval(1000L, sendSoilMoistureData); // Kirim data setiap 1 detik
  timer.setInterval(60000L, checkTimers);        // Periksa interval penyiraman setiap menit
}

void loop() {
  Blynk.run();    // Jalankan Blynk
  timer.run();    // Jalankan timer
}

// Fungsi untuk mengirim data kelembapan tanah ke Blynk
void sendSoilMoistureData() {
  soilMoistureValue = analogRead(SOIL_MOISTURE_PIN); // Baca sensor
  Blynk.virtualWrite(V0, soilMoistureValue);        // Kirim data ke Virtual Pin V0

  // Tampilkan data di LCD
  lcd.setCursor(0, 0);
  lcd.print("Kelembapan: ");
  lcd.setCursor(12, 0);
  lcd.print(soilMoistureValue);
  lcd.print("   "); // Hapus sisa karakter lama

  // Tampilkan status pompa
  lcd.setCursor(0, 1);
  if (pumpState) {
    lcd.print("Pompa: ON     ");
  } else if (fertilizerPumpState) {
    lcd.print("Pupuk: ON     ");
  } else {
    lcd.print("Pompa: OFF    ");
  }
}

// Fungsi untuk mengatur interval penyiraman
void checkTimers() {
  unsigned long currentTime = millis();

  // Periksa mode otomatis dan kondisi tanah
  if (!manualMode && soilMoistureValue > thresholdDry && currentTime - autoWaterLockTime >= autoWaterInterval) {
    digitalWrite(RELAY_PIN, LOW); // Hidupkan pompa air
    pumpState = true;
    timer.setTimeout(waterPumpDuration * 1000L, []() { // Matikan pompa setelah durasi
      digitalWrite(RELAY_PIN, HIGH);
      pumpState = false;
      autoWaterLockTime = millis(); // Kunci penyiraman hingga interval berikutnya
    });
  }

  // Periksa penyiraman pupuk
  unsigned long fertilizerIntervalMillis = 7UL * 24 * 3600000UL; // 1 minggu dalam milidetik
  if (currentTime - lastFertilizerPumpTime >= fertilizerIntervalMillis) {
    digitalWrite(RELAY_FERTILIZER_PIN, LOW); // Hidupkan pompa pupuk
    fertilizerPumpState = true;
    timer.setTimeout(fertilizerPumpDuration * 1000L, []() { // Matikan pompa setelah durasi
      digitalWrite(RELAY_FERTILIZER_PIN, HIGH);
      fertilizerPumpState = false;
    });
    lastFertilizerPumpTime = currentTime; // Perbarui waktu terakhir pompa pupuk
  }
}

// Fungsi untuk mengaktifkan mode manual
BLYNK_WRITE(V1) {
  int pinValue = param.asInt(); // Membaca nilai dari Virtual Pin V1
  manualMode = pinValue;        // Aktifkan mode manual berdasarkan nilai tombol

  // Tampilkan status mode manual di LCD
  lcd.setCursor(0, 1);
  if (manualMode) {
    lcd.print("Mode Manual AKTIF");
  } else {
    lcd.print("Mode Manual MATI ");
  }
}

// Fungsi untuk mengontrol pompa air secara manual
BLYNK_WRITE(V2) {
  if (manualMode) { // Hanya kontrol manual jika mode manual aktif
    int pinValue = param.asInt(); // Membaca nilai dari Virtual Pin V2
    if (pinValue) {
      digitalWrite(RELAY_PIN, LOW); // Hidupkan pompa air
      pumpState = true;
    } else {
      digitalWrite(RELAY_PIN, HIGH); // Matikan pompa air
      pumpState = false;
    }
  }
}

// Fungsi untuk mengontrol pompa pupuk secara manual
BLYNK_WRITE(V3) {
  int pinValue = param.asInt(); // Membaca nilai dari Virtual Pin V3
  if (pinValue) {
    digitalWrite(RELAY_FERTILIZER_PIN, LOW); // Hidupkan pompa pupuk
    fertilizerPumpState = true;
    timer.setTimeout(fertilizerPumpDuration * 1000L, []() { // Matikan pompa pupuk setelah durasi
      digitalWrite(RELAY_FERTILIZER_PIN, HIGH);
      fertilizerPumpState = false;
    });
  } else {
    digitalWrite(RELAY_FERTILIZER_PIN, HIGH); // Matikan pompa pupuk
    fertilizerPumpState = false;
  }
}

