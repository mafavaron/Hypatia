// Hypatia_Logger - Data acquisition and preliminary processing
//                  for Metek GmbH.
//
// Copyright 2019 by Servizi Territorio srl

#include <Arduino.h>
#include "wiring_private.h"
#include <Wire.h>
#include <RTClib.h>
#include <SD.h>
#include <string.h>

RTC_DS3231 rtc;
uint8_t  iOldYear;
uint8_t  iOldMonth;
uint8_t  iOldDay;
uint8_t  iOldHour;
uint8_t  iOldMinute;
uint8_t  iOldSecond;
uint32_t iOldTime;

// Input pin definitions
const int COLD_START =  2;
const int SPEED_0    = 10;
const int SPEED_1    =  5;
const int SPEED_2    =  4;
const int SPEED_3    =  3;
const int USA_1      =  9;
const int APPLI_0    =  6;
const int APPLI_1    =  7;
const int APPLI_2    =  8;

// Auxiliary serial
Uart mySerial(&sercom3, 0, 1, SERCOM_RX_PAD_1, UART_TX_PAD_0); // Create the new UART instance assigning it to pin 0 and 1

// Other constants
const String sEmpty = "      "; // 6 spaces: the USA-1 way to say "invalid data"

// Configuration flags and values
bool isColdStart;
int  iSpeed;
int  iUsa1;
int  iAveragingTime;

// Internal state
// -1- SD card management
File fData;
bool bCanWrite;
// -1- Statistical accumulators (for emission)
unsigned long iNumData;
unsigned long iNumValid;
unsigned long iNumTimeout;
float rSumU;
float rSumV;
float rSumW;
float rSumT;
float rSumUU;
float rSumVV;
float rSumWW;
float rSumTT;
float rSumUV;
float rSumUW;
float rSumVW;
float rSumUT;
float rSumVT;
float rSumWT;
// -1- Statistical accumulators (for 1s display)
unsigned long iNumSecData;
unsigned long iNumSecValid;
unsigned long iNumSecTimeout;
float rSecSumU;
float rSecSumV;
float rSecSumW;
float rSecSumT;


void getConfig(void) {
  
  isColdStart = (digitalRead(COLD_START) == LOW);

  int iSpeedIdx = (digitalRead(SPEED_0) == LOW) + 
                  ((digitalRead(SPEED_1) == LOW) << 1) + 
                  ((digitalRead(SPEED_2) == LOW) << 2) + 
                  ((digitalRead(SPEED_3) == LOW) << 3);

  switch(iSpeedIdx) {
    case 0:
      iSpeed =    300;
      break;
    case 1:
      iSpeed =    600;
      break;
    case 2:
      iSpeed =   1200;
      break;
    case 3:
      iSpeed =   2400;
      break;
    case 4:
      iSpeed =   4800;
      break;
    case 5:
      iSpeed =   9600;
      break;
    case 6:
      iSpeed =  19200;
      break;
    case 7:
      iSpeed =  38400;
      break;
    case 8:
      iSpeed =  57600;
      break;
    case 9:
      iSpeed = 115200;
      break;
    default:
      iSpeed =   9600;
      break;
  }
  iSpeed = 19200;

  int iUsa1 = (digitalRead(USA_1) == LOW);
  
  int iAveragingTimeIdx = (digitalRead(APPLI_0) == LOW) + 
                          ((digitalRead(APPLI_1) == LOW) << 1) + 
                          ((digitalRead(APPLI_2) == LOW) << 2);

  switch(iAveragingTimeIdx) {
    case 0:
      iAveragingTime = 1*60;
      break;
    case 1:
      iAveragingTime = 5*60;
      break;
    case 2:
      iAveragingTime = 10*60;
      break;
    case 3:
      iAveragingTime = 12*60;
      break;
    case 4:
      iAveragingTime = 15*60;
      break;
    case 5:
      iAveragingTime = 20*60;
      break;
    case 6:
      iAveragingTime = 30*60;
      break;
    case 7:
      iAveragingTime = 60*60;
      break;
  }
  iAveragingTime = 60;

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
  rSumU          = 0.0f;
  rSumV          = 0.0f;
  rSumW          = 0.0f;
  rSumT          = 0.0f;
  rSumUU         = 0.0f;
  rSumVV         = 0.0f;
  rSumWW         = 0.0f;
  rSumTT         = 0.0f;
  rSumUV         = 0.0f;
  rSumUW         = 0.0f;
  rSumVW         = 0.0f;
  rSumUT         = 0.0f;
  rSumVT         = 0.0f;
  rSumWT         = 0.0f;
}


void cleanSecCounters(void) {
  iNumSecData       = 0L;
  iNumSecValid      = 0L;
  iNumSecTimeout    = 0L;
  rSecSumU          = 0.0f;
  rSecSumV          = 0.0f;
  rSecSumW          = 0.0f;
  rSecSumT          = 0.0f;
}


// Attach the interrupt handler to the SERCOM
void SERCOM3_Handler()
{
  mySerial.IrqHandler();
}


// ******************** //
// * Preparation Part * //
// ******************** //

void setup () {

  // Configure pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(COLD_START,  INPUT_PULLUP);
  pinMode(SPEED_0,     INPUT_PULLUP);
  pinMode(SPEED_1,     INPUT_PULLUP);
  pinMode(SPEED_2,     INPUT_PULLUP);
  pinMode(SPEED_3,     INPUT_PULLUP);
  pinMode(USA_1,       INPUT_PULLUP);
  pinMode(APPLI_0,     INPUT_PULLUP);
  pinMode(APPLI_1,     INPUT_PULLUP);
  pinMode(APPLI_2,     INPUT_PULLUP);

  // Auxiliary serial
  mySerial.begin(9600);
  pinPeripheral(0, PIO_SERCOM); // Assign RX function to pin 0
  pinPeripheral(1, PIO_SERCOM); // Assign TX function to pin 1

  // Read configuration from DIP and non-DIP switches
  getConfig();

  Serial.begin(9600);
  delay(2000); // Wait console to settle up

  if(!rtc.begin()) {
    Serial.println("Missing or failed RTC");
    notifyFailure();
  }
  DateTime now = rtc.now();
  iOldTime = now.unixtime() / iAveragingTime;

  // Set sonic port as required by configuration
  Serial1.begin(iSpeed);
  Serial1.setTimeout(100L);

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

  // Dump DIP switch configuration
  Serial.print("Ultrasonic anemometer assumed port speed: ");
  Serial.println(iSpeed);
  Serial.print("Averaging time:                           ");
  Serial.println(iAveragingTime);
  Serial.println("");

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
  long int iVx, iVy, iVz, iT;

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
      if(iUsa1 != 0) {
        // USA-1 and old uSonic-3
        iVx = sV.toInt();
        iVy = sU.toInt();
      }
      else {
        // New uSonic-3
        iVx = sU.toInt();
        iVy = sV.toInt();
      }
      iVz = sW.toInt();
      iT  = sT.toInt();
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

  // Print accumulators, if time has come
  if(iOldTime != iCurrentTime / iAveragingTime) {

    uint32_t iFinalTimeRounded   = (iCurrentTime / iAveragingTime) * iAveragingTime;
    uint32_t iInitialTimeRounded = iFinalTimeRounded - iAveragingTime;

    DateTime dateFrom(iInitialTimeRounded);
    DateTime dateTo(iFinalTimeRounded);
    char sDateFrom[32];
    sprintf(
      sDateFrom,
      "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
      dateFrom.year(),
      dateFrom.month(),
      dateFrom.day(),
      dateFrom.hour(),
      dateFrom.minute(),
      dateFrom.second()
    );
    char sDateTo[32];
    sprintf(
      sDateTo,
      "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d",
      dateTo.year(),
      dateTo.month(),
      dateTo.day(),
      dateTo.hour(),
      dateTo.minute(),
      dateTo.second()
    );
    char buffer[512];
    sprintf(
      buffer,
      "%s, %s, %d, %d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",
      sDateFrom,
      sDateTo,
      iNumData,
      iNumValid,
      iNumTimeout,
      rSumU, rSumV, rSumW, rSumT,
      rSumUU, rSumVV, rSumWW, rSumTT,
      rSumUV, rSumUW, rSumVW,
      rSumUT, rSumVT, rSumWT
    );
    Serial.println(buffer);
    mySerial.println(buffer);

    // Prepare next step
    iOldTime = iCurrentTime / iAveragingTime;
    cleanCounters();
    
  }
  
  // Accumulate counters
  if(canAccumulate) {
    iNumValid++;
    iNumSecValid++;
    rSumU += iVx / 100.0f;
    rSumV += iVy / 100.0f;
    rSumW += iVz / 100.0f;
    rSumT += iT  / 100.0f;
    rSumUU += (iVx / 100.0f) * (iVx / 100.0f);
    rSumVV += (iVy / 100.0f) * (iVy / 100.0f);
    rSumWW += (iVz / 100.0f) * (iVz / 100.0f);
    rSumTT += (iT  / 100.0f) * (iT  / 100.0f);
    rSumUV += (iVx / 100.0f) * (iVy / 100.0f);
    rSumUW += (iVx / 100.0f) * (iVz / 100.0f);
    rSumVW += (iVy / 100.0f) * (iVz / 100.0f);
    rSumUT += (iVx / 100.0f) * (iT  / 100.0f);
    rSumVT += (iVy / 100.0f) * (iT  / 100.0f);
    rSumWT += (iVz / 100.0f) * (iT  / 100.0f);
    rSecSumU += iVx / 100.0f;
    rSecSumV += iVy / 100.0f;
    rSecSumW += iVz / 100.0f;
    rSecSumT += iT  / 100.0f;
  }

  // Print 1s means
  if(bGoOnPrinting) {

    float rAvgVx;
    float rAvgVy;
    float rAvgVz;
    float rAvgT;
    
    if(iNumValid > 0) {
      rAvgVx = rSecSumU / (float)iNumSecValid;
      rAvgVy = rSecSumV / (float)iNumSecValid;
      rAvgVz = rSecSumW / (float)iNumSecValid;
      rAvgT  = rSecSumT / (float)iNumSecValid;
    }
    else {
      rAvgVx = -9999.90f;
      rAvgVy = -9999.90f;
      rAvgVz = -9999.90f;
      rAvgT  = -9999.90f;
    }

    char buffer[128];
    sprintf(
      buffer,
      "%s, %4d, %4d, %4d, %6.2f, %6.2f, %6.2f, %6.2f",
      dateTime, iNumSecData, iNumSecValid, iNumSecTimeout,
      rAvgVx, rAvgVy, rAvgVz,
      rAvgT
    );
    Serial.println(buffer);

    cleanSecCounters();
    
  }

}
