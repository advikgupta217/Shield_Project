#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

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

TwoWire MPUWire = TwoWire(1);


// ================= MPU6050 REGISTERS =================
#define PWR_MGMT_1   0x6B
#define ACCEL_XOUT_H 0x3B


// ================= STEP COUNTER SETTINGS =================

// Acceleration threshold for detecting a step
float STEP_THRESHOLD = 1.20;

// Minimum time between two steps
unsigned long STEP_DELAY = 300;


// ================= VARIABLES =================

int stepCount = 0;

float previousAcceleration = 1.0;

unsigned long lastStepTime = 0;


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

  // MPU6050 default ±2g range
  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;
}


// =================================================
// DISPLAY STEP COUNT
// =================================================

void displaySteps() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("STEP COUNTER");

  // Step count
  display.setTextSize(3);
  display.setCursor(10, 18);
  display.print(stepCount);

  display.setTextSize(1);
  display.setCursor(10, 52);
  display.println("STEPS");

  display.display();
}


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(115200);


  // ================= OLED =================

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    Serial.println("OLED not found!");

    while (1);
  }


  // ================= MPU6050 =================

  MPUWire.begin(MPU_SDA, MPU_SCL);

  MPUWire.beginTransmission(MPU6050_ADDR);

  byte error = MPUWire.endTransmission();


  if (error == 0) {

    Serial.println("MPU6050 found!");

  } else {

    Serial.println("MPU6050 not found!");

    while (1);
  }


  // ================= WAKE MPU6050 =================

  MPUWire.beginTransmission(MPU6050_ADDR);

  MPUWire.write(PWR_MGMT_1);
  MPUWire.write(0x00);

  MPUWire.endTransmission();


  // ================= START DISPLAY =================

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println("MPU6050");

  display.setCursor(0, 15);
  display.println("STEP COUNTER");

  display.setCursor(0, 35);
  display.println("Starting...");

  display.display();

  delay(1500);

  displaySteps();
}


// =================================================
// LOOP
// =================================================

void loop() {

  float ax;
  float ay;
  float az;


  // Read acceleration
  readAcceleration(ax, ay, az);


  // Calculate total acceleration
  float totalAcceleration = sqrt(
    ax * ax +
    ay * ay +
    az * az
  );


  // =================================================
  // STEP DETECTION
  // =================================================

  unsigned long currentTime = millis();


  // Detect acceleration peak
  if (
    totalAcceleration > STEP_THRESHOLD &&
    previousAcceleration <= STEP_THRESHOLD &&
    currentTime - lastStepTime > STEP_DELAY
  ) {

    stepCount++;

    lastStepTime = currentTime;

    Serial.print("Step detected!  Steps = ");
    Serial.println(stepCount);

    displaySteps();
  }


  previousAcceleration = totalAcceleration;


  // =================================================
  // DEBUG
  // =================================================

  Serial.print("Acceleration: ");
  Serial.println(totalAcceleration);


  delay(50);
}