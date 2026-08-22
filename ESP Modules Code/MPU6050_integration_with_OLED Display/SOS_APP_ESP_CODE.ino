#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// =====================================================
// OLED
// =====================================================

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


// =====================================================
// MPU6050
// =====================================================

#define MPU_SDA 21
#define MPU_SCL 22

#define MPU6050_ADDR 0x68

TwoWire MPUWire = TwoWire(1);

#define PWR_MGMT_1    0x6B
#define ACCEL_XOUT_H  0x3B


// =====================================================
// BLE
// =====================================================

#define SERVICE_UUID \
"4fafc201-1fb5-459e-8fcc-c5c9c331914b"

#define CHARACTERISTIC_UUID \
"beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;

bool deviceConnected = false;


// =====================================================
// SERVER CALLBACKS
// =====================================================
int stepCount = 0;
class ServerCallbacks : public BLEServerCallbacks {

  void onConnect(
    BLEServer *server
  ) override {

    deviceConnected = true;

    Serial.println();
    Serial.println("==============================");
    Serial.println("PHONE CONNECTED!");
    Serial.println("==============================");

    display.clearDisplay();

    display.setTextColor(
      SSD1306_WHITE
    );

    display.setTextSize(1);

    display.setCursor(0, 0);

    display.println(
      "BLE CONNECTED"
    );

    display.setCursor(0, 20);

    display.println(
      "STEPS:"
    );

    display.setTextSize(2);

    display.setCursor(45, 17);

    display.println(
      stepCount
    );

    display.display();
  }


  void onDisconnect(
    BLEServer *server
  ) override {

    deviceConnected = false;

    Serial.println(
      "PHONE DISCONNECTED"
    );

    delay(100);

    BLEDevice::startAdvertising();

    Serial.println(
      "BLE advertising restarted"
    );
  }
};


// =====================================================
// STEP COUNTER
// =====================================================


#define STEP_THRESHOLD 0.25

#define MIN_STEP_TIME 350

#define MAX_STEP_TIME 2000

#define FILTER_SIZE 5

float accelerationBuffer[FILTER_SIZE];

int bufferIndex = 0;

float previousFiltered = 0;

bool rising = false;

unsigned long lastStepTime = 0;


// =====================================================
// FALL DETECTION
// =====================================================

#define FREEFALL_THRESHOLD 0.55

#define IMPACT_THRESHOLD 2.3

#define FALL_WINDOW 1000

#define STILL_TIME 1500

bool possibleFall = false;

bool impactDetected = false;

unsigned long freeFallTime = 0;

unsigned long impactTime = 0;

bool fallDetected = false;


// =====================================================
// READ MPU6050
// =====================================================

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


  ax = rawX / 16384.0;

  ay = rawY / 16384.0;

  az = rawZ / 16384.0;


  return true;
}


// =====================================================
// SEND BLE DATA
// =====================================================

void sendBLEData() {

  if (!deviceConnected) {
    return;
  }


  String data =
    "STEPS:" +
    String(stepCount) +
    ",FALL:" +
    String(fallDetected ? 1 : 0);


  pCharacteristic->setValue(
    data.c_str()
  );


  pCharacteristic->notify();


  Serial.print(
    "BLE -> "
  );

  Serial.println(
    data
  );
}


// =====================================================
// OLED NORMAL DISPLAY
// =====================================================

void displaySteps() {

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
    "SAFETY MONITOR"
  );

  display.setCursor(
    0,
    12
  );

  if (deviceConnected) {

    display.println(
      "BLE: CONNECTED"
    );

  } else {

    display.println(
      "BLE: WAITING"
    );
  }


  display.setTextSize(3);

  display.setCursor(
    10,
    27
  );

  display.print(
    stepCount
  );


  display.setTextSize(1);

  display.setCursor(
    10,
    55
  );

  display.println(
    "STEPS"
  );


  display.display();
}


// =====================================================
// OLED FALL DISPLAY
// =====================================================

void displayFall() {

  display.clearDisplay();

  display.setTextColor(
    SSD1306_WHITE
  );

  display.setTextSize(2);

  display.setCursor(
    15,
    5
  );

  display.println(
    "WARNING!"
  );

  display.setTextSize(1);

  display.setCursor(
    20,
    32
  );

  display.println(
    "FALL DETECTED"
  );

  display.setCursor(
    20,
    48
  );

  display.println(
    "CHECK PHONE"
  );

  display.display();
}


// =====================================================
// STEP DETECTION
// =====================================================

void detectSteps(
  float acceleration
) {

  float dynamicAcceleration =
    acceleration - 1.0;


  accelerationBuffer[
    bufferIndex
  ] = dynamicAcceleration;


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


  float filtered =
    sum / FILTER_SIZE;


  unsigned long now =
    millis();


  if (
    filtered >
    previousFiltered
  ) {

    rising = true;
  }


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


      if (
        lastStepTime == 0 ||
        (
          timeSinceLastStep >
          MIN_STEP_TIME &&

          timeSinceLastStep <
          MAX_STEP_TIME
        )
      ) {

        stepCount++;

        lastStepTime =
          now;


        Serial.print(
          "STEP = "
        );

        Serial.println(
          stepCount
        );


        displaySteps();

        sendBLEData();
      }
    }
  }


  previousFiltered =
    filtered;
}


// =====================================================
// FALL DETECTION
// =====================================================

void detectFall(
  float acceleration
) {

  unsigned long now =
    millis();


  // Free fall

  if (
    acceleration <
    FREEFALL_THRESHOLD
  ) {

    if (!possibleFall) {

      possibleFall = true;

      freeFallTime =
        now;

      Serial.println(
        "Possible free fall"
      );
    }
  }


  // Timeout

  if (
    possibleFall &&
    now - freeFallTime >
    FALL_WINDOW
  ) {

    possibleFall = false;
  }


  // Impact

  if (
    possibleFall &&
    acceleration >
    IMPACT_THRESHOLD
  ) {

    possibleFall = false;

    impactDetected = true;

    impactTime =
      now;

    Serial.println(
      "Impact detected"
    );
  }


  // Confirm fall

  if (
    impactDetected &&
    now - impactTime >
    STILL_TIME
  ) {

    impactDetected = false;

    fallDetected = true;


    Serial.println();
    Serial.println(
      "=============================="
    );

    Serial.println(
      "FALL DETECTED!"
    );

    Serial.println(
      "=============================="
    );


    displayFall();


    sendBLEData();


    // Keep fall state for 5 seconds

    delay(5000);


    fallDetected = false;

    displaySteps();

    sendBLEData();
  }
}


// =====================================================
// SETUP
// =====================================================

void setup() {

  Serial.begin(
    115200
  );


  // ===================================================
  // OLED
  // ===================================================

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


  // ===================================================
  // MPU6050
  // ===================================================

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


  // Wake MPU

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


  // Initialize filter

  for (
    int i = 0;
    i < FILTER_SIZE;
    i++
  ) {

    accelerationBuffer[i] =
      0;
  }


  // ===================================================
  // BLE
  // ===================================================

  BLEDevice::init(
    "ESP32_Safety_Device"
  );


  BLEServer *server =
    BLEDevice::createServer();


  server->setCallbacks(
    new ServerCallbacks()
  );


  BLEService *service =
    server->createService(
      SERVICE_UUID
    );


  pCharacteristic =
    service->createCharacteristic(

      CHARACTERISTIC_UUID,

      BLECharacteristic::PROPERTY_READ |
      BLECharacteristic::PROPERTY_NOTIFY
    );


  pCharacteristic->addDescriptor(
    new BLE2902()
  );


  pCharacteristic->setValue(
    "STEPS:0,FALL:0"
  );


  service->start();


  // Advertise the SERVICE UUID

  BLEAdvertising *advertising =
    BLEDevice::getAdvertising();


  advertising->addServiceUUID(
    SERVICE_UUID
  );


  advertising->setScanResponse(
    true
  );


  advertising->setMinPreferred(
    0x06
  );

  advertising->setMinPreferred(
    0x12
  );


  BLEDevice::startAdvertising();


  Serial.println();
  Serial.println(
    "=============================="
  );

  Serial.println(
    "BLE STARTED"
  );

  Serial.println(
    "Name: ESP32_Safety_Device"
  );

  Serial.println(
    "Waiting for Android..."
  );

  Serial.println(
    "=============================="
  );


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
    "SAFETY MONITOR"
  );

  display.setCursor(
    0,
    18
  );

  display.println(
    "BLE READY"
  );

  display.setCursor(
    0,
    34
  );

  display.println(
    "WAITING FOR PHONE"
  );

  display.display();


  delay(2000);

  displaySteps();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  float ax;
  float ay;
  float az;


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


  float acceleration =
    sqrt(
      ax * ax +
      ay * ay +
      az * az
    );


  // Fall detection

  if (!fallDetected) {

    detectFall(
      acceleration
    );
  }


  // Step detection

  if (!fallDetected) {

    detectSteps(
      acceleration
    );
  }


  delay(40);
}