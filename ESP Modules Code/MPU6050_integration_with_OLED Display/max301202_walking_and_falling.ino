#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// =================================================
// OLED
// =================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 15
#define OLED_SCL 16

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);


// =================================================
// MPU6050
// =================================================

#define MPU_SDA 21
#define MPU_SCL 22

#define MPU6050_ADDR 0x68

TwoWire MPUWire = TwoWire(1);

#define PWR_MGMT_1    0x6B
#define ACCEL_XOUT_H  0x3B


// =================================================
// STEP DETECTION SETTINGS
// =================================================

// Higher = less sensitive
#define STEP_THRESHOLD 0.25

// Minimum time between steps
#define MIN_STEP_TIME 350

// Maximum time between walking steps
#define MAX_STEP_TIME 2000

// Moving average filter
#define FILTER_SIZE 5


// =================================================
// FALL DETECTION SETTINGS
// =================================================

// Free-fall threshold
// Acceleration below this means possible free fall
#define FREEFALL_THRESHOLD 0.55

// Impact threshold
// Large acceleration after free fall
#define IMPACT_THRESHOLD 2.3

// Maximum time between free-fall and impact
#define FALL_WINDOW 1000

// Time to check for inactivity after impact
#define STILL_TIME 1500

// Movement threshold during inactivity check
#define MOVEMENT_THRESHOLD 0.20


// =================================================
// STEP VARIABLES
// =================================================

int stepCount = 0;

float accelerationBuffer[FILTER_SIZE];

int bufferIndex = 0;

float previousFiltered = 0;

bool rising = false;

unsigned long lastStepTime = 0;


// =================================================
// FALL VARIABLES
// =================================================

bool possibleFall = false;

bool impactDetected = false;

unsigned long freeFallTime = 0;

unsigned long impactTime = 0;

bool fallDetected = false;


// =================================================
// READ MPU6050
// =================================================

bool readAcceleration(
  float &ax,
  float &ay,
  float &az
) {

  MPUWire.beginTransmission(
    MPU6050_ADDR
  );

  MPUWire.write(
    ACCEL_XOUT_H
  );

  if (
    MPUWire.endTransmission(false)
    != 0
  ) {
    return false;
  }


  if (
    MPUWire.requestFrom(
      MPU6050_ADDR,
      6,
      true
    ) != 6
  ) {
    return false;
  }


  int16_t rawX =
    (MPUWire.read() << 8) |
    MPUWire.read();

  int16_t rawY =
    (MPUWire.read() << 8) |
    MPUWire.read();

  int16_t rawZ =
    (MPUWire.read() << 8) |
    MPUWire.read();


  // MPU6050 ±2g
  ax = rawX / 16384.0;

  ay = rawY / 16384.0;

  az = rawZ / 16384.0;


  return true;
}


// =================================================
// TOTAL ACCELERATION
// =================================================

float getAccelerationMagnitude(
  float ax,
  float ay,
  float az
) {

  return sqrt(
    ax * ax +
    ay * ay +
    az * az
  );
}


// =================================================
// MOVING AVERAGE FILTER
// =================================================

float filterAcceleration(
  float acceleration
) {

  accelerationBuffer[bufferIndex] =
    acceleration;

  bufferIndex++;

  if (
    bufferIndex >= FILTER_SIZE
  ) {
    bufferIndex = 0;
  }


  float sum = 0;


  for (
    int i = 0;
    i < FILTER_SIZE;
    i++
  ) {

    sum +=
      accelerationBuffer[i];
  }


  return sum / FILTER_SIZE;
}


// =================================================
// DISPLAY NORMAL SCREEN
// =================================================

void displaySteps() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  // Title
  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "STEP COUNTER"
  );


  // Step count
  display.setTextSize(3);

  display.setCursor(
    10,
    18
  );

  display.print(
    stepCount
  );


  // Steps
  display.setTextSize(1);

  display.setCursor(
    10,
    52
  );

  display.println(
    "STEPS"
  );


  display.display();
}


// =================================================
// FALL WARNING SCREEN
// =================================================

void displayFallWarning() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(2);

  display.setCursor(
    25,
    5
  );

  display.println(
    "WARNING"
  );


  display.setTextSize(1);

  display.setCursor(
    20,
    30
  );

  display.println(
    "FALL DETECTED!"
  );


  display.setCursor(
    15,
    48
  );

  display.println(
    "CHECK YOURSELF"
  );


  display.display();
}


// =================================================
// FALL DETECTION
// =================================================

void detectFall(
  float acceleration
) {

  unsigned long now =
    millis();


  // -------------------------------------------------
  // STEP 1: FREE FALL
  // -------------------------------------------------

  if (
    acceleration <
    FREEFALL_THRESHOLD
  ) {

    if (!possibleFall) {

      possibleFall = true;

      freeFallTime = now;

      Serial.println(
        "Possible free fall!"
      );
    }
  }


  // -------------------------------------------------
  // STEP 2: CANCEL OLD FREE FALL
  // -------------------------------------------------

  if (
    possibleFall &&
    now - freeFallTime >
    FALL_WINDOW
  ) {

    possibleFall = false;

    Serial.println(
      "Free fall timeout"
    );
  }


  // -------------------------------------------------
  // STEP 3: IMPACT
  // -------------------------------------------------

  if (
    possibleFall &&
    acceleration >
    IMPACT_THRESHOLD
  ) {

    possibleFall = false;

    impactDetected = true;

    impactTime = now;

    Serial.println(
      "IMPACT DETECTED!"
    );
  }


  // -------------------------------------------------
  // STEP 4: WAIT FOR INACTIVITY
  // -------------------------------------------------

  if (
    impactDetected &&
    now - impactTime >
    STILL_TIME
  ) {

    // If we reach here after
    // an impact, declare fall

    fallDetected = true;

    impactDetected = false;

    Serial.println(
      "***** FALL DETECTED *****"
    );


    displayFallWarning();
  }
}


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(
    115200
  );

  delay(500);


  // =================================================
  // OLED
  // =================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  Wire.setClock(
    100000
  );


  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      0x3C
    )
  ) {

    Serial.println(
      "OLED not found!"
    );

    while (1);
  }


  Serial.println(
    "OLED found!"
  );


  // =================================================
  // MPU6050
  // =================================================

  MPUWire.begin(
    MPU_SDA,
    MPU_SCL
  );

  MPUWire.setClock(
    100000
  );


  MPUWire.beginTransmission(
    MPU6050_ADDR
  );


  if (
    MPUWire.endTransmission()
    != 0
  ) {

    Serial.println(
      "MPU6050 not found!"
    );

    while (1);
  }


  Serial.println(
    "MPU6050 found!"
  );


  // =================================================
  // WAKE MPU6050
  // =================================================

  MPUWire.beginTransmission(
    MPU6050_ADDR
  );

  MPUWire.write(
    PWR_MGMT_1
  );

  MPUWire.write(
    0x00
  );

  MPUWire.endTransmission();


  // =================================================
  // INITIALIZE FILTER
  // =================================================

  for (
    int i = 0;
    i < FILTER_SIZE;
    i++
  ) {

    accelerationBuffer[i] =
      0;
  }


  // =================================================
  // START SCREEN
  // =================================================

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);


  display.setCursor(
    0,
    0
  );

  display.println(
    "MPU6050"
  );


  display.setCursor(
    0,
    15
  );

  display.println(
    "STEP + FALL"
  );


  display.setCursor(
    0,
    35
  );

  display.println(
    "Starting..."
  );


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


  // =================================================
  // READ MPU6050
  // =================================================

  if (
    !readAcceleration(
      ax,
      ay,
      az
    )
  ) {

    Serial.println(
      "MPU6050 read error!"
    );

    delay(50);

    return;
  }


  // =================================================
  // TOTAL ACCELERATION
  // =================================================

  float acceleration =
    getAccelerationMagnitude(
      ax,
      ay,
      az
    );


  // =================================================
  // FALL DETECTION
  // =================================================

  if (!fallDetected) {

    detectFall(
      acceleration
    );
  }


  // =================================================
  // STEP DETECTION
  // =================================================

  if (!fallDetected) {

    float dynamicAcceleration =
      acceleration - 1.0;


    float filtered =
      filterAcceleration(
        dynamicAcceleration
      );


    unsigned long now =
      millis();


    // Rising signal

    if (
      filtered >
      previousFiltered
    ) {

      rising = true;
    }


    // Falling after peak

    if (
      rising &&
      filtered <
      previousFiltered
    ) {

      float peak =
        previousFiltered;


      rising = false;


      if (
        peak >
        STEP_THRESHOLD
      ) {

        unsigned long
        timeSinceLastStep =
          now - lastStepTime;


        // First step

        if (
          lastStepTime == 0
        ) {

          stepCount++;

          lastStepTime =
            now;

          displaySteps();


          Serial.print(
            "STEP = "
          );

          Serial.println(
            stepCount
          );
        }


        // Normal walking

        else if (
          timeSinceLastStep >
          MIN_STEP_TIME &&

          timeSinceLastStep <
          MAX_STEP_TIME
        ) {

          stepCount++;

          lastStepTime =
            now;

          displaySteps();


          Serial.print(
            "STEP = "
          );

          Serial.println(
            stepCount
          );
        }
      }
    }


    previousFiltered =
      filtered;
  }


  // =================================================
  // DEBUG
  // =================================================

  Serial.print(
    "Acceleration: "
  );

  Serial.print(
    acceleration
  );

  Serial.print(
    " | Steps: "
  );

  Serial.print(
    stepCount
  );

  Serial.print(
    " | Fall: "
  );

  Serial.println(
    fallDetected
  );


  delay(40);
}