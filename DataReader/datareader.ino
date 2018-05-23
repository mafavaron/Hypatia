// Simple (and "very wrong") data reader.
//
// This Arduino sketch is part of the Hypatia project.
//
// Parts:
//
// - An Arduino (an "Uno" is just sufficient; I'm using a Feather M0 AdaLogger, by AdaFruit)
// - A DHT-22 temperature and relative humidity sensor (good for indoor, mainstream accuracy)
// - A breadboard
//
// Libraries (if you have not yet installed):
//
// - DHT sensor library (from AdaFruit)
// - Adafruit Unified Sensor library (it may be necessary to install
//   this dependency of DHT lib by hand).
//
// What you may expect from this sketch:
//
// 1) A very very simple data acquisition example
//
// 2) Indirectly, a test that your DHT-22 and Arduino more-or-less work
//
// This is open-source software, covered by the MIT license.
//
//
// Written by: Mauri Favaron

// Library imports:
#include <DHT.h>    // -1- DHT-11, DHT-22 temp/relh sensors

// Global variable, referring to the DHT-22 used in this project
#define DHT_PIN 6
#define DHT_TYPE DHT22
DHT dht(DHT_PIN, DHT_TYPE);

// Global variables, containing the values of temperature and
// relative humidity just as coming from the DHT-22 sensor
float rTemperature;
float rRelHumidity;

// Initialization
void setup() {
  Serial.begin(9600); // Will be used to monitor the system activity
  dht.begin();        // Clean-up and start DHT-22
}

void loop() {

  // Wait some time (the amount should be larger than the interval
  // the slowest sensor takes to get a new reading; 1s is sufficient
  // for the DHT-22.
  delay(1000);

  // Get readings
  rTemperature = dht.readTemperature();
  rRelHumidity = dht.readHumidity();

  // Show us what the sensor did read
  Serial.print("Ta: ");
  Serial.print(rTemperature);
  Serial.print(" °C    RH: ");
  Serial.print(rRelHumidity);
  Serial.println(" %");
  
}
