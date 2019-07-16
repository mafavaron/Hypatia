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
// -1- Statistical accumulators (for emission)
unsigned long iNumData;
unsigned long iNumValid;
unsigned long iNumTimeout;
// -1- Statistical accumulators (for 1s display)
unsigned long iNumSecData;
unsigned long iNumSecValid;
unsigned long iNumSecTimeout;


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


// Send a Morse-encoded SOS, forever (until user fix things and resets)
void notifyFailure(void) {
  while(true) {
    int i;
    for(i=0; i<3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(70);
    }
    for(i=0; i<3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);
      delay(70);
    }
    for(i=0; i<3; i++) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(100);
      digitalWrite(LED_BUILTIN, LOW);
      delay(70);
    }
    delay(2000);
  }
}


void cleanCounters(void) {
  iNumData       = 0L;
  iNumValid      = 0L;
  iNumTimeout    = 0L;
}


void cleanSecCounters(void) {
  iNumSecData       = 0L;
  iNumSecValid      = 0L;
  iNumSecTimeout    = 0L;
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

  Serial.begin(9600);
  delay(2000); // Wait console to settle up

  if(!rtc.begin()) {
    Serial.println("Missing or failed RTC");
    notifyFailure();
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
    notifyFailure();
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
    notifyFailure();
  }

  // Update counters
  cleanCounters();
  cleanSecCounters();
    
}

// ******************* //
// * Functional Part * //
// ******************* //

void loop () {
  
  // Loop basic variables
  char dateTime[20];

  // Wait for next string, with timeout
  String sonicLine = Serial1.readStringUntil('\n');
  int numChars = sonicLine.length();
  bool canParseString = false;
  if(numChars > 0) {
    iNumData++;
    iNumSecData++;
    if(numChars == 42) {
      canParseString = sonicLine[2] == 'x';
    }
  }
  else iNumTimeout++;

  // Perform string parsing
  bool canAccumulate = false;
  if(canParseString) {
    String sU = sonicLine.substring(5, 11);
    String sV = sonicLine.substring(15, 21);
    String sW = sonicLine.substring(25, 31);
    String sT = sonicLine.substring(35, 41);
    if(sU != sEmpty) {
      // USA-1 and old uSonic-3
      int iVx = sV.toInt();
      int iVy = sU.toInt();
      int iVz = sW.toInt();
      int iT  = sT.toInt();
      canAccumulate = true;
    }
  }
  
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
      notifyFailure();
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
  if(bCanWrite && sonicLine.length() > 0 && sonicLine.length() < 50) {
    char cvLine[64];
    digitalWrite(LED_BUILTIN, HIGH);
    sprintf(cvLine, "%d, %s", iSecondTimeStamp, sonicLine.c_str());
    byte bytesWritten = fData.write(cvLine);
    delay(3);
    digitalWrite(LED_BUILTIN, LOW);
    if(bytesWritten < strlen(cvLine)) {
      Serial.print("Bytes in string to write: ");
      Serial.print(strlen(cvLine));
      Serial.print("  Bytes actually written to SD: ");
      Serial.print(bytesWritten);
      Serial.println("  => SD Card failure");
      notifyFailure();
    }
  }

  // Accumulate counters
  if(canAccumulate) {
    iNumValid++;
    iNumSecValid++;
  }

  // Print 1s means, and clean their accumulators
  if(bGoOnPrinting) {

    // Print

    // Clean
    cleanSecCounters();
    
  }

}
