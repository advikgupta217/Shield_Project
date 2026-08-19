#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======================================
// OLED
// ======================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 5
#define OLED_SCL 4

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

// ======================================
// LM35
// ======================================
#define LM35_PIN 1

// ESP32 ADC reference voltage
#define ADC_VOLTAGE 3.3

// ESP32 ADC resolution = 12 bits
#define ADC_MAX 4095.0

// ======================================
// SETUP
// ======================================
void setup() {

  Serial.begin(115200);

  // OLED I2C
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    while (true);
  }

  // Set ADC resolution
  analogReadResolution(12);

  // Startup screen
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(25, 20);
  display.println("LM35 SENSOR");

  display.setCursor(25, 35);
  display.println("Initializing...");

  display.display();

  delay(2000);
}

// ======================================
// LOOP
// ======================================
void loop() {

  // Read ADC
  int adcValue = analogRead(LM35_PIN);

  // Convert ADC value to voltage
  float voltage = (adcValue / ADC_MAX) * ADC_VOLTAGE;

  // LM35 output = 10 mV per degree Celsius
  float temperatureC = voltage * 100.0;

  // Serial Monitor
  Serial.print("ADC: ");
  Serial.print(adcValue);

  Serial.print("  Voltage: ");
  Serial.print(voltage, 3);

  Serial.print(" V  Temperature: ");
  Serial.print(temperatureC, 1);

  Serial.println(" C");

  // ======================================
  // OLED
  // ======================================

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(25, 5);
  display.println("TEMPERATURE");

  display.drawLine(0, 18, 128, 18, SSD1306_WHITE);

  display.setTextSize(2);
  display.setCursor(10, 32);

  display.print(temperatureC, 1);
  display.print(" C");

  display.display();

  delay(1000);
}