// Hypatia_Logger - Data acquisition and preliminary processing of temperature and relative humidity data.
//
// Copyright 2019 by Servizi Territorio srl

#include <Arduino.h>
#include "wiring_private.h"
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <string.h>
#include "Adafruit_SHT31.h"

RTC_DS3231 rtc;
uint8_t  iOldYear;
uint8_t  iOldMonth;
uint8_t  iOldDay;
uint8_t  iOldHour;
uint8_t  iOldMinute;
uint8_t  iOldSecond;
uint32_t iOldTime;

Adafruit_SHT31 sht31 = Adafruit_SHT31();

// Input pin definitions
const int COLD_START =  12;

// Other constants
const String sEmpty = "      "; // 6 spaces: the USA-1 way to say "invalid data"

// Configuration flags and values
bool isColdStart;

// Internal state
// -1- SD card management
File fData;
bool bCanWrite;
// -1- Statistical accumulators (for 1s display)
float         rSumTemp;
float         rSumRelH;
unsigned long iNumSecData;
unsigned long iNumSecValid;


void getConfig(void) {
  
  isColdStart = (digitalRead(COLD_START) == LOW);

}


bool getUserDateTime(void) {

  bool isOK;
  
  Serial.setTimeout(30000L);
  uint16_t iNewYear   = 0;
  uint8_t  iNewMonth  = 0;
  uint8_t  iNewDay    = 0;
  uint8_t  iNewHour   = 0;
  uint8_t  iNewMinute = 0;
  uint8_t  iNewSecond = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  String startTime = Serial.readStringUntil('\n');
  String sNewYear   = startTime.substring(0,   4);
  String sNewMonth  = startTime.substring(5,   7);
  String sNewDay    = startTime.substring(8,  10);
  String sNewHour   = startTime.substring(11, 13);
  String sNewMinute = startTime.substring(14, 16);
  String sNewSecond = startTime.substring(17, 19);
  iNewYear   = sNewYear.toInt();
  iNewMonth  = sNewMonth.toInt();
  iNewDay    = sNewDay.toInt();
  iNewHour   = sNewHour.toInt();
  iNewMinute = sNewMinute.toInt();
  iNewSecond = sNewSecond.toInt();
  if(
    iNewYear < 0 || 
    iNewMonth < 1 || iNewMonth > 12 || 
    iNewDay < 1 || iNewDay > 31 || 
    iNewHour < 0 || iNewHour > 23 || 
    iNewMinute < 0 || iNewMinute > 59 ||
    iNewSecond < 0 || iNewSecond > 59
  ) {
    isOK = false;
  }
  else {
    rtc.adjust(DateTime(iNewYear, iNewMonth, iNewDay, iNewHour, iNewMinute, iNewSecond));
    isOK = true;
  }
  digitalWrite(LED_BUILTIN, LOW);

  return isOK;
  
}


void cleanSecCounters(void) {
  rSumTemp          = 0.0f;
  rSumRelH          = 0.0f;
  iNumSecData       = 0L;
  iNumSecValid      = 0L;
}


void notifyFailure(long complement=500, String msg="") {
  while(true) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000-complement);
    digitalWrite(LED_BUILTIN, LOW);
    delay(complement);
    Serial.println(msg);
  }
}


// ******************** //
// * Preparation Part * //
// ******************** //

void setup() {
  
  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(COLD_START,  INPUT_PULLUP);

  // Read configuration from DIP and non-DIP switches
  getConfig();

  // Setup console
  Serial.begin(9600);
  delay(2000); // Wait console to settle up

  // Start SHT31
  if(!sht31.begin(0x44)) {
    Serial.println("Missing SHT31");
    notifyFailure(300, "SHT31 not responding");
  }

  // Start RTC
  if(!rtc.begin()) {
    Serial.println("Missing or failed RTC");
    notifyFailure(300, "RTC not responding");
  }
  DateTime now = rtc.now();

  // Get user date and time, if requested by configuration
  if(isColdStart) {
    Serial.println("---> Cold start: Assigning date and time manually");
    Serial.println("     You have 30 seconds to enter an ISO date and time value (YYYY-MM-DD HH:MM:SS)");
    bool isOK = getUserDateTime();
  }
  else {
    if(rtc.lostPower()) {
      Serial.println("---> Warm start attempted, but RTC got a power fail: get user date and time");
      Serial.println("     You have 30 seconds to enter an ISO date and time value (YYYY-MM-DD HH:MM:SS)");
      bool isOK = getUserDateTime();
    }
    else {
      Serial.println("---> Warm start: Using RTC date and time as they are");
    }
  }

  // SD card initialization
  Serial.println("--> Starting SD");
  bool canWrite = SD.begin();
  if(!canWrite) {
    Serial.println("Connect to SD card failed");
    notifyFailure(100, "SD card not operating");
  }

}

void loop() {
  delay(1000);
}
