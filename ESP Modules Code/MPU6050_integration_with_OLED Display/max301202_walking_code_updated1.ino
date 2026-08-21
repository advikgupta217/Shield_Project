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

// Walking acceleration amplitude
// Higher = less sensitive
#define STEP_THRESHOLD 0.25

// Minimum time between steps
#define MIN_STEP_TIME 350

// Maximum time allowed between steps
// Used to identify continuous walking
#define MAX_STEP_TIME 2000

// Number of samples used for smoothing
#define FILTER_SIZE 5


// =================================================
// VARIABLES
// =================================================

int stepCount = 0;

float accelerationBuffer[FILTER_SIZE];

int bufferIndex = 0;

float previousFiltered = 0;

bool rising = false;

unsigned long lastStepTime = 0;

int consecutiveSteps = 0;


// =================================================
// READ MPU6050
// =================================================

bool readAcceleration(
  float &ax,
  float &ay,
  float &az
) {

  MPUWire.beginTransmission(MPU6050_ADDR);

  MPUWire.write(ACCEL_XOUT_H);

  if (MPUWire.endTransmission(false) != 0) {
    return false;
  }

  if (MPUWire.requestFrom(
        MPU6050_ADDR,
        6,
        true
      ) != 6) {

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


  ax = rawX / 16384.0;
  ay = rawY / 16384.0;
  az = rawZ / 16384.0;

  return true;
}


// =================================================
// CALCULATE ACCELERATION
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

  if (bufferIndex >= FILTER_SIZE) {
    bufferIndex = 0;
  }

  float sum = 0;

  for (int i = 0; i < FILTER_SIZE; i++) {

    sum += accelerationBuffer[i];
  }

  return sum / FILTER_SIZE;
}


// =================================================
// DISPLAY
// =================================================

void displaySteps() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );


  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println(
    "STEP COUNTER"
  );


  display.setTextSize(3);

  display.setCursor(10, 18);

  display.print(
    stepCount
  );


  display.setTextSize(1);

  display.setCursor(10, 52);

  display.println(
    "STEPS"
  );


  display.display();
}


// =================================================
// SETUP
// =================================================

void setup() {

  Serial.begin(115200);

  delay(500);


  // =================================================
  // OLED
  // =================================================

  Wire.begin(
    OLED_SDA,
    OLED_SCL
  );

  Wire.setClock(100000);


  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        0x3C
      )) {

    Serial.println(
      "OLED not found!"
    );

    while (1);
  }


  // =================================================
  // MPU6050
  // =================================================

  MPUWire.begin(
    MPU_SDA,
    MPU_SCL
  );

  MPUWire.setClock(100000);


  MPUWire.beginTransmission(
    MPU6050_ADDR
  );

  if (MPUWire.endTransmission() != 0) {

    Serial.println(
      "MPU6050 not found!"
    );

    while (1);
  }


  Serial.println(
    "MPU6050 found!"
  );


  // Wake MPU6050

  MPUWire.beginTransmission(
    MPU6050_ADDR
  );

  MPUWire.write(
    PWR_MGMT_1
  );

  MPUWire.write(0x00);

  MPUWire.endTransmission();


  // =================================================
  // INITIALIZE FILTER
  // =================================================

  for (int i = 0; i < FILTER_SIZE; i++) {

    accelerationBuffer[i] =
      1.0;
  }


  // =================================================
  // START SCREEN
  // =================================================

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println(
    "MPU6050"
  );

  display.setCursor(0, 15);

  display.println(
    "STEP COUNTER"
  );

  display.setCursor(0, 35);

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
  // READ SENSOR
  // =================================================

  if (!readAcceleration(
        ax,
        ay,
        az
      )) {

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
  // REMOVE GRAVITY
  // =================================================

  float dynamicAcceleration =
    acceleration - 1.0;


  // =================================================
  // FILTER
  // =================================================

  float filtered =
    filterAcceleration(
      dynamicAcceleration
    );


  unsigned long now =
    millis();


  // =================================================
  // STEP DETECTION
  // =================================================

  // Detect rising acceleration

  if (
    filtered >
    previousFiltered
  ) {

    rising = true;
  }


  // Detect falling after a peak

  if (
    rising &&

    filtered <
    previousFiltered
  ) {

    float peak =
      previousFiltered;


    rising = false;


    // Check if peak is large enough

    if (
      peak >
      STEP_THRESHOLD
    ) {

      unsigned long timeSinceLastStep =
        now - lastStepTime;


      // First step

      if (
        lastStepTime == 0
      ) {

        stepCount++;

        lastStepTime = now;

        consecutiveSteps = 1;

        displaySteps();

        Serial.println(
          "STEP 1"
        );
      }


      // Normal walking step

      else if (
        timeSinceLastStep >
        MIN_STEP_TIME &&

        timeSinceLastStep <
        MAX_STEP_TIME
      ) {

        stepCount++;

        lastStepTime = now;

        consecutiveSteps++;

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


  // =================================================
  // DEBUG
  // =================================================

  Serial.print(
    "Dynamic: "
  );

  Serial.print(
    filtered
  );

  Serial.print(
    " | Steps: "
  );

  Serial.println(
    stepCount
  );


  delay(40);
}