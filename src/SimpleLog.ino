// Basic demo for reading Humidity and Temperature from HTS221, tested on an
// Arduino Nano 33 BLE
#include <RTCZero.h>
#include <Wire.h>
#include <Adafruit_HTS221.h>
#include <Adafruit_Sensor.h>

// Time base
RTCZero rtc;

// MicroSD reader
#include <SD.h>
const int chipSelect = SS1;

// MicroSD file
File    logfile;
bool    bCardAvailable = false;
bool    bFileIsOpen    = false;
bool    bLogToSerial   = true;

// Commands
String sBuffer="";

// For SPI mode, we need a CS pin
#define HTS_CS_OBS 6
#define HTS_CS_MOD 7
// For software-SPI mode we need SCK/MOSI/MISO pins
#define HTS_SCK   9
#define HTS_MISO 10
#define HTS_MOSI  8

// Scheduling-related quantities
long int deltaTime;
long int oneSecond = 1000L;
long int currentTime;
unsigned long int previousMillis = 0L;

unsigned long int time_ms;

// Date and time management

long int JulianDay(int iYear, int iMonth, int iDay) {
 
  // Preliminary estimate of Julian day
  const long int DATE_REFORM_DAY =  588829L;  // 15 October 1582, with 31-days months
  const long int BASE_DAYS       = 1720995L;
  const double   YEAR_DURATION   =     365.25;
  const double   MONTH_DURATION  =      30.6001;
  int iAuxYear;
  int iAuxMonth;
  if(iMonth > 2) {
    iAuxYear  = iYear;
    iAuxMonth = iMonth + 1;
  }
  else {
    iAuxYear  = iYear  -  1;
    iAuxMonth = iMonth + 13;
  }
  long int iFirstGuessJDay = floor(YEAR_DURATION * iAuxYear) + floor(MONTH_DURATION * iAuxMonth) + iDay + BASE_DAYS;

  // Correct estimate of Julian day for post-reform dates
  long int iNumDays = iDay + 31L * iMonth + 372L * iYear;
  long int iJulianDay;
  if(iNumDays >= DATE_REFORM_DAY) {
    int iCentury = iAuxYear / 100;
    iJulianDay = iFirstGuessJDay - iCentury + iCentury/4 + 2;
  }
  else {
    iJulianDay = iFirstGuessJDay;
  }

  // Yield result
  return(iJulianDay);
  
}

void UnpackDate(long int iJulianDay, int* iYear, int* iMonth, int* iDay) {
  
  long int iDeviation;
  long int iPreJulianDay;
  long int iPostJulianDay;
  long int iYearIndex;
  long int iMonthIndex;
  long int iDayIndex;

  const long int LIMIT_JULIAN_DAY = 2299161L;
  const long int CORRECTION_DAYS  =    1524L;

  // Unwind effects of Pope Gregorius reform
  if(iJulianDay >= LIMIT_JULIAN_DAY) {
    iDeviation = floor(((iJulianDay - 1867216L) - 0.25) / 36524.25);
    iPreJulianDay = iJulianDay + iDeviation - iDeviation/4L + 1L;
  }
  else {
    iPreJulianDay = iJulianDay;
  }
  iPostJulianDay = iPreJulianDay + CORRECTION_DAYS;

  // Compute time indices
  iYearIndex  = floor(6680.0+((iPostJulianDay-2439870L)-122.1)/365.25);
  iDayIndex   = 365L*iYearIndex + iYearIndex/4L;
  iMonthIndex = floor((iPostJulianDay - iDayIndex)/30.6001);

  // Deduce date from indices.
  *iDay = iPostJulianDay - floor(30.6001*iMonthIndex) - iDayIndex;
  if(iMonthIndex > 13L) {
      *iMonth = iMonthIndex - 13L;
  }
  else {
      *iMonth = iMonthIndex - 1L;
  }
  *iYear = iYearIndex - 4715L;
  if(*iMonth > 2) *iYear -= 1;
  
}


long int PackTime(int iYear, int iMonth, int iDay, int iHour, int iMinute, int iSecond) {
  long int iJulianDay = JulianDay(iYear, iMonth, iDay) - 2440588L;
  long int iTime      = iJulianDay * 86400L + iSecond + 60L*(iMinute + 60L*iHour);
  return(iTime);
}


long int Now() {
  int iYear, iMonth, iDay, iHour, iMinute, iSecond;
  iYear   = rtc.getYear();
  iMonth  = rtc.getMonth();
  iDay    = rtc.getDay();
  iHour   = rtc.getHours();
  iMinute = rtc.getMinutes();
  iSecond = rtc.getSeconds();
  return PackTime(iYear, iMonth, iDay, iHour, iMinute, iSecond);
}


void Now(int* iYear, int* iMonth, int* iDay, int* iHour, int* iMinute, int* iSecond) {
  *iYear   = rtc.getYear();
  *iMonth  = rtc.getMonth();
  *iDay    = rtc.getDay();
  *iHour   = rtc.getHours();
  *iMinute = rtc.getMinutes();
  *iSecond = rtc.getSeconds();
}


void UnpackTime(long long int iTime, int* iYear, int* iMonth, int* iDay, int* iHour, int* iMinute, int* iSecond) {

  long int iJulianDay;
  long int iTimeSeconds;

  // Check parameters
  if(iTime < 0L) {
    *iYear   = 1970;
    *iMonth  =    1;
    *iDay    =    1;
    *iHour   =    0;
    *iMinute =    0;
    *iSecond =    0;
    return;
  }

  // Isolate date and time
  iJulianDay   = iTime/86400L + 2440588L;
  iTimeSeconds = iTime % 86400L;
  
  // Process the date part
  UnpackDate(iJulianDay, iYear, iMonth, iDay);
  
  // Extract time from the time part
  *iSecond = iTimeSeconds % 60L;
  iTimeSeconds = iTimeSeconds / 60L;
  *iMinute = iTimeSeconds % 60L;
  *iHour   = iTimeSeconds / 60L;
  
}


// Callback function to set date and time in MicroSD files
void fileDateTime(uint16_t* date, uint16_t* time) {

  *date = FAT_DATE(rtc.getYear(), rtc.getMonth(), rtc.getDay());
  *time = FAT_TIME(rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  
}

Adafruit_HTS221 hts_0;
Adafruit_HTS221 hts_1;
void setup(void) {

  pinMode(LED_BUILTIN, OUTPUT);
  
  Serial.begin(115200);
  // while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  // Try to initialize!
  if (!hts_0.begin_SPI(HTS_CS_OBS, HTS_SCK, HTS_MISO, HTS_MOSI)) {
    Serial.println("Failed to find base HTS221 '0' chip");
    while (1) { delay(10); }
  }
  if (!hts_1.begin_SPI(HTS_CS_MOD, HTS_SCK, HTS_MISO, HTS_MOSI)) {
    Serial.println("Failed to find derived HTS221 '1' chip");
    while (1) { delay(10); }
  }

  // Start RTC (without initializing it: reference date is 2000-01-01T00:00:00)
  rtc.begin();

/*
  Serial.print("Data rate set to: ");
  switch (hts_0.getDataRate()) {
   case HTS221_RATE_ONE_SHOT: Serial.println("0: One Shot"); break;
   case HTS221_RATE_1_HZ: Serial.println("0: 1 Hz"); break;
   case HTS221_RATE_7_HZ: Serial.println("0: 7 Hz"); break;
   case HTS221_RATE_12_5_HZ: Serial.println("0: 12.5 Hz"); break;
  }
  switch (hts_1.getDataRate()) {
   case HTS221_RATE_ONE_SHOT: Serial.println("1: One Shot"); break;
   case HTS221_RATE_1_HZ: Serial.println("1: 1 Hz"); break;
   case HTS221_RATE_7_HZ: Serial.println("1: 7 Hz"); break;
   case HTS221_RATE_12_5_HZ: Serial.println("1: 12.5 Hz"); break;
  }
*/

  // Start MicroSD, and check it meanwhile
  if (SD.begin(chipSelect)) {
    bCardAvailable = true;
    SdFile::dateTimeCallback(fileDateTime);
  }
            
  // Scheduling-related variables
  currentTime = 0L;

}

void loop() {

  unsigned long currentMillis = millis();
  if(currentMillis - previousMillis >= oneSecond) {

    previousMillis = currentMillis;

    // Gather data from the two sensors
    sensors_event_t temp_0;
    sensors_event_t humidity_0;
    hts_0.getEvent(&humidity_0, &temp_0);// populate temp and humidity objects with fresh data
    float RelH_0 = humidity_0.relative_humidity;
    sensors_event_t temp_1;
    sensors_event_t humidity_1;
    hts_1.getEvent(&humidity_1, &temp_1);// populate temp and humidity objects with fresh data
    float RelH_1 = humidity_1.relative_humidity;

    // Write data to serial port, if requested
    if(bLogToSerial) {
      Serial.print(RelH_0); Serial.print(", ");
      Serial.println(RelH_1);
    }

    // Log data to SD card file, if requested
    if(logfile) {
      digitalWrite(LED_BUILTIN, HIGH);
      char sTimeStamp[32];
      long int iTimeStamp = Now();
      int iYear, iMonth, iDay, iHour, iMinute, iSecond;
      Now(&iYear, &iMonth, &iDay, &iHour, &iMinute, &iSecond);
      sprintf(sTimeStamp, "%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d", iYear, iMonth, iDay, iHour, iMinute, iSecond);
      logfile.print(sTimeStamp); logfile.print(",");
      logfile.print(RelH_0); logfile.print(",");
      logfile.println(RelH_1);
      logfile.flush();
      digitalWrite(LED_BUILTIN, LOW);
    }

  }

  delay(100);

  // Check whether some command has been received
  if(Serial.available() > 0) {

    char ch = Serial.read();
    sBuffer += ch;
    if(ch == '\r') {

      Serial.println(sBuffer);

      // Parse and understand command
      sBuffer.trim();
      switch(sBuffer[0]) {

        case 'T':
        case 't':

          // Set time (used when time stamping recorded data)

          // Parse command (S yyyy-mm-ddThh:mm:ss <file_name>)
          if(sBuffer.length() == 21) {
            byte iYear   = sBuffer.substring(2,6).toInt() % 100;
            byte iMonth  = sBuffer.substring(7,9).toInt();
            byte iDay    = sBuffer.substring(10,12).toInt();
            byte iHour   = sBuffer.substring(13,15).toInt();
            byte iMinute = sBuffer.substring(16,18).toInt();
            byte iSecond = sBuffer.substring(19).toInt();
            if(iYear <= 0 || iYear >= 2999 || iMonth < 1 || iMonth > 12 || iDay < 1 || iDay > 31 || iHour < 0 || iHour > 23 || iMinute < 0 || iMinute > 59 || iSecond < 0 || iSecond > 60) {
              Serial.println("Invalid time string - Should be 'T yyyy-mm-dd hh:mm:ss' (without the ' characters)");
            }
            else {
              // Set RTC
              rtc.setYear(iYear);
              rtc.setMonth(iMonth);
              rtc.setDay(iDay);
              rtc.setHours(iHour);
              rtc.setMinutes(iMinute);
              rtc.setSeconds(iSecond);
              Serial.println("Real-time clock set to assigned date/time");
            }
          }
          else {
            Serial.println("Error in string - Should be 'T yyyy-mm-dd hh:mm:ss' (without the ' characters)");
          }

          break;          

        case 'S':
        case 's':

          // Start new recording

          // Check card is present: if not so start it
          if(!bCardAvailable) {
            if(SD.begin(chipSelect)) bCardAvailable = true;
          }

          // Parse command (S <file_name>)
          if(bCardAvailable) {
            if(sBuffer.length() > 2) {
              String sFileName = sBuffer.substring(2);

              logfile = SD.open(sFileName, FILE_WRITE);

            }
            else {
              Serial.println("Error in command");
            }
          }
          break;

        case 'E':
        case 'e':

          // Terminate (a possible) recording
          if(bCardAvailable && logfile) logfile.close();
          bLogToSerial = false;

          break;

        case 'L':
        case 'l':

          // Terminate (a possible) recording
          bLogToSerial != bLogToSerial;
          if(bLogToSerial) {
            Serial.println("Logging data to serial");            
          }
          else {
            Serial.println("Serial data log suspended");
          }

          break;

      }
      Serial.println(sBuffer);

      sBuffer = "";

    }

  }

}
