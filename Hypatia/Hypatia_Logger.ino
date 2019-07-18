// Hypatia_Logger - Data acquisition and preliminary processing of temperature and relative humidity data.
//
// Copyright 2019 by Servizi Territorio srl

#include <Arduino.h>
#include "wiring_private.h"
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <string.h>
#include <Adafruit_SHT31.h>

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


// ******************** //
// * Preparation Part * //
// ******************** //

void setup () {

  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(COLD_START,  INPUT_PULLUP);

  // Read configuration from DIP and non-DIP switches
  getConfig();

  // Setup console
  Serial.begin(9600);
  delay(2000); // Wait console to settle up

  // Start SHT31
  sht31.begin(0x44);

  // Start RTC
  if(!rtc.begin()) {
    Serial.println("Missing or failed RTC");
    digitalWrite(LED_BUILTIN, HIGH);
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
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // Open first file in write mode, gathering the current time
  // from the RTC, for uniformity
  char dirName[8];
  char fileName[10];
  uint16_t uiYear  = now.year();
  uint8_t uiMonth  = now.month();
  uint8_t uiDay    = now.day();
  uint8_t uiHour   = now.hour();
  uint8_t uiMinute = now.minute();
  uint8_t uiSecond = now.second();
  sprintf(dirName, "%4.4d%2.2d", uiYear, uiMonth);
  sprintf(fileName, "%4.4d%2.2d%2.2d.%2.2d", uiYear, uiMonth, uiDay, uiHour);
  SD.mkdir(dirName);
  String sFile = dirName;
  sFile += "/";
  sFile += fileName;
  fData = SD.open(sFile.c_str(), FILE_WRITE);
  if(fData) bCanWrite = true;
  else bCanWrite = false;
  if(!bCanWrite) {
    Serial.println("File creation on SD card failed");
    digitalWrite(LED_BUILTIN, HIGH);
  }

  // Initialize counters
  cleanSecCounters();

}

// ******************* //
// * Functional Part * //
// ******************* //

void loop () {
  
  // Loop basic variables
  char dateTime[20];

  // Perform readings
  float rTemp = sht31.readTemperature();
  float rRelH = sht31.readHumidity();
  
  // Get full time stamp
  DateTime now     = rtc.now();
  uint16_t iYear   = now.year();
  uint8_t  iMonth  = now.month();
  uint8_t  iDay    = now.day();
  uint8_t  iHour   = now.hour();
  uint8_t  iMinute = now.minute();
  uint8_t  iSecond = now.second();
  int iSecondTimeStamp = iSecond + 60*iMinute;
  uint32_t iCurrentTime = now.unixtime();
  
  // Flush data to file in order to ensure data is streamed regularly to SD
  bool bGoOnPrinting;
  if(iOldMinute != iMinute || iOldSecond != iSecond) {

    // Ensure pending data are written to file
    fData.flush();

    // Show users the current state
    sprintf(dateTime, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%.2d", iYear, iMonth, iDay, iHour, iMinute, iSecond);
    bGoOnPrinting = true;
    
  }
  else bGoOnPrinting = false;

  // Generate directory and file names, and open file on SD card
  if(iOldYear != iYear || iOldMonth != iMonth || iOldDay != iDay || iOldHour != iHour) {

    // Generate directory and file names; ensure directory exists
    char dirName[8];
    char fileName[10];
    sprintf(dirName, "%4.4d%2.2d", iYear, iMonth);
    sprintf(fileName, "%4.4d%2.2d%2.2d.%2.2d", iYear, iMonth, iDay, iHour);
    SD.mkdir(dirName);
    String sFile = dirName;
    sFile += "/";
    sFile += fileName;
    fData.close();

    // Create file, and position counter so that writes will occur
    // in append mode, if the file exists already
    fData = SD.open(sFile.c_str(), FILE_WRITE);
    if(fData) {
      bCanWrite = true;
      fData.seek(fData.size());
    }
    else {
      Serial.println("File creation on SD card failed");
      digitalWrite(LED_BUILTIN, HIGH);
    }

  }
  
  // Update time stamp
  iOldYear   = iYear;
  iOldMonth  = iMonth;
  iOldDay    = iDay;
  iOldHour   = iHour;
  iOldMinute = iMinute;
  iOldSecond = iSecond;
  
  // Save data to SD file
  if(bCanWrite) {
    char cvLine[64];
    digitalWrite(LED_BUILTIN, HIGH);
    sprintf(cvLine, "%d, %f, %f", iSecondTimeStamp, rTemp, rRelH);
    byte bytesWritten = fData.write(cvLine);
    delay(3);
    digitalWrite(LED_BUILTIN, LOW);
    if(bytesWritten < strlen(cvLine)) {
      Serial.print("Bytes in string to write: ");
      Serial.print(strlen(cvLine));
      Serial.print("  Bytes actually written to SD: ");
      Serial.print(bytesWritten);
      Serial.println("  => SD Card failure");
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  // Accumulate counters
  iNumSecData++;
  if(!isnan(rTemp) && !isnan(rRelH)) {
    rSumTemp += rTemp;
    rSumRelH += rRelH;
    iNumSecValid++;
  }

  // Print 1s means, and clean their accumulators
  if(bGoOnPrinting) {

    // Print
    if(iNumSecValid > 0L) {
      Serial.print("Ta = ");
      Serial.print(rSumTemp / iNumSecValid);
      Serial.print(" °C,   R = ");
      Serial.print(rSumRelH / iNumSecValid);
      Serial.println(" %");
    }

    // Clean
    cleanSecCounters();
    
  }

  // Loop delay
  delay(100);

}
