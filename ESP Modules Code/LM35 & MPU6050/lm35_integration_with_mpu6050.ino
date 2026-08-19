#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =================================================
// OLED
// =================================================
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

// =================================================
// LM35
// =================================================
#define LM35_PIN 1

#define ADC_VOLTAGE 3.3
#define ADC_MAX 4095.0

// =================================================
// MPU6050
// =================================================
#define MPU_SDA 7
#define MPU_SCL 6

#define MPU6050_ADDR 0x68

TwoWire MPUWire = TwoWire(1);

// MPU6050 registers
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B

// =================================================
// STEP COUNTER
// =================================================
int stepCount = 0;

float previousMagnitude = 1.0;

bool stepDetected = false;

unsigned long lastStepTime = 0;

// Minimum time between two steps
const unsigned long STEP_DELAY = 300;

// Sensitivity for step detection
const float STEP_THRESHOLD = 0.20;

// =================================================
// WRITE MPU6050 REGISTER
// =================================================
void writeMPURegister(byte reg, byte value) {

  MPUWire.beginTransmission(MPU6050_ADDR);
  MPUWire.write(reg);
  MPUWire.write(value);
  MPUWire.endTransmission();
}

// =================================================
// READ MPU6050 ACCELERATION
// =================================================
void readAcceleration(float &ax, float &ay, float &az) {

  MPUWire.beginTransmission(MPU6050_ADDR);
  MPUWire.write(ACCEL_XOUT_H);
  MPUWire.endTransmission(false);

  MPUWire.requestFrom(MPU6050_ADDR, 6, true);

  int16_t rawX = (MPUWire.read() << 8) | MPUWire.read();
  int16_t rawY = (MPUWire.read() << 8) | MPUWire.read();
  int16_t rawZ = (MPUWire.read() << 8) | MPUWire.read();

  // Default ±2g sensitivity
  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;
}

// =================================================
// SETUP
// =================================================
void setup() {

  Serial.begin(115200);

  // =================================================
  // OLED I2C
  // =================================================
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED not found!");

    while (true);
  }

  // =================================================
  // LM35 ADC
  // =================================================
  analogReadResolution(12);

  // =================================================
  // MPU6050 I2C
  // =================================================
  MPUWire.begin(MPU_SDA, MPU_SCL);

  delay(100);

  // Wake MPU6050
  writeMPURegister(PWR_MGMT_1, 0x00);

  delay(100);

  // =================================================
  // STARTUP DISPLAY
  // =================================================
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(25, 15);
  display.println("HEALTH MONITOR");

  display.setCursor(30, 30);
  display.println("LM35 + MPU6050");

  display.setCursor(35, 45);
  display.println("Starting...");

  display.display();

  delay(2000);
}

// =================================================
// LOOP
// =================================================
void loop() {

  // =================================================
  // READ LM35
  // =================================================

  int adcValue = analogRead(LM35_PIN);

  float voltage =
    (adcValue / ADC_MAX) * ADC_VOLTAGE;

  // LM35 = 10mV per °C
  float temperatureC =
    voltage * 100.0;


  // =================================================
  // READ MPU6050
  // =================================================

  float ax, ay, az;

  readAcceleration(ax, ay, az);


  // =================================================
  // CALCULATE ACCELERATION MAGNITUDE
  // =================================================

  float magnitude =
    sqrt(
      ax * ax +
      ay * ay +
      az * az
    );


  // =================================================
  // STEP DETECTION
  // =================================================

  float change =
    abs(magnitude - previousMagnitude);

  unsigned long currentTime = millis();

  if (change > STEP_THRESHOLD &&
      !stepDetected &&
      currentTime - lastStepTime > STEP_DELAY) {

    stepCount++;

    lastStepTime = currentTime;

    stepDetected = true;
  }

  // Reset detection after acceleration settles
  if (change < STEP_THRESHOLD * 0.5) {

    stepDetected = false;
  }

  previousMagnitude = magnitude;


  // =================================================
  // SERIAL MONITOR
  // =================================================

  Serial.print("Temperature: ");
  Serial.print(temperatureC, 1);
  Serial.print(" C");

  Serial.print(" | Steps: ");
  Serial.print(stepCount);

  Serial.print(" | Accel: ");
  Serial.println(magnitude, 2);


  // =================================================
  // OLED DISPLAY
  // =================================================

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);


  // Temperature
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("TEMPERATURE");

  display.setTextSize(2);

  display.setCursor(0, 12);

  display.print(temperatureC, 1);
  display.print(" C");


  // Divider
  display.drawLine(
    0,
    34,
    128,
    34,
    SSD1306_WHITE
  );


  // Steps
  display.setTextSize(1);

  display.setCursor(0, 39);
  display.println("STEP COUNT");

  display.setTextSize(2);

  display.setCursor(0, 49);

  display.print(stepCount);


  display.display();


  delay(100);
}