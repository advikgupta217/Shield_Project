#include <Wire.h>
#include "MAX30105.h"

MAX30105 particleSensor;

#define MAX_SDA 5
#define MAX_SCL 4

void setup()
{
  Serial.begin(115200);

  // Start I2C
  Wire.begin(MAX_SDA, MAX_SCL);

  Serial.println("Initializing MAX30102...");

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    Serial.println("MAX30102 not found!");
    while (1);
  }

  Serial.println("MAX30102 detected.");

  // Sensor configuration
  particleSensor.setup();

  // LED brightness
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeIR(0x0A);

  Serial.println("Place your finger on the sensor.");
}

void loop()
{
  long irValue = particleSensor.getIR();

  Serial.print("IR: ");
  Serial.println(irValue);

  delay(100);
}