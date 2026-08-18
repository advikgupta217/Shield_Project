#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 20

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= MPU6050 =================
#define MPU_SDA 2
#define MPU_SCL 1

#define MPU6050_ADDR 0x68

// Second I2C bus for MPU6050
TwoWire MPUWire = TwoWire(1);


void setup() {
  Serial.begin(115200);

  // ================= OLED I2C =================
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (1);
  }

  // ================= MPU6050 I2C =================
  MPUWire.begin(MPU_SDA, MPU_SCL);

  // Check MPU6050 connection
  MPUWire.beginTransmission(MPU6050_ADDR);
  byte error = MPUWire.endTransmission();

  if (error == 0) {
    Serial.println("MPU6050 found!");
  } else {
    Serial.println("MPU6050 not found!");
  }

  // ================= OLED DISPLAY =================
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Hello!");

  display.setTextSize(1);
  display.setCursor(0, 30);
  display.println("ESP32 OLED");

  display.setCursor(0, 45);
  display.println("MPU6050 Ready");

  display.display();
}


void loop() {
}