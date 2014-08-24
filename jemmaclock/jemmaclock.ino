/* 

Jemma Clock for Arduino
MIT License

Copyright (c) 2014 Patrick Tudor
https://www.gemmagps.com/clock/

*/

#define JEMMA_VERSION "Jemma Clock v1.0"
#define JEMMA_COPYRIGHT "(c)Patrick Tudor"
#define DEBUG 1

#ifndef _HEADERS_JEMMA
#define _HEADERS_JEMMA

#include <Time.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>
#include <TinyGPS++.h>
#include <avr/wdt.h> // watchdog timer
#include "freeram.h"

#endif

// list of timezones
// http://en.wikipedia.org/wiki/List_of_UTC_time_offsets

namespace TimeZoneList {
  typedef struct  {
   int    id;         // 1..99
   long   tzoffset;   // 28800
   char   *tzname;    // "UTC-8 Pacific"
  } Timezones;

 static const Timezones tzlist[40] = {
      { 0, -43200, "UTC-12"},            // Y
      { 1, -39600, "UTC-11"},            // X
      { 2, -36000, "UTC-10 Honolulu"},   // W
      { 3, -34200, "UTC-9:30"},          // V+
      { 4, -32400, "UTC-9 Anchorage"},   // V
      { 5, -28800, "UTC-8 Tijuana"},     // U
      { 6, -25200, "UTC-7 Denver"},      // T
      { 7, -21600, "UTC-6 Chicago"},     // S
      { 8, -18000, "UTC-5 New York"},    // R
      { 9, -16200, "UTC-4:30"},          // Q+
      {10, -14400, "UTC-4 Virgin Is."},  // Q
      {11, -12600, "UTC-3:30"},          // P+
      {12, -10800, "UTC-3"},             // P
      {13, -7200,  "UTC-2"},             // O
      {14, -3600,  "UTC-1 Azores"},      // N
      {15, 0,      "UTC London"},        // Z ±0
      {16, 3600,   "UTC+1 Berlin"},      // A
      {17, 7200,   "UTC+2 Cairo"},       // B
      {18, 10800,  "UTC+3 Baghdad"},     // C
      {19, 12600,  "UTC+3:30 Tehran"},   // C+
      {20, 14400,  "UTC+4 Moscow"},      // D
      {21, 16200,  "UTC+4:30 Kabul"},    // D+
      {22, 18000,  "UTC+5 Karachi"},     // E
      {23, 19800,  "UTC+5:30 Delhi"},    // E+
      {24, 20700,  "UTC+5:45 Nepal"},    // E+
      {25, 21600,  "UTC+6"},             // F
      {26, 23400,  "UTC+6:30"},          // F+
      {27, 25200,  "UTC+7 Jakarta"},     // G 
      {28, 28800,  "UTC+8 Perth"},       // H
      {29, 31500,  "UTC+8:45 Eucla"},    // H+
      {30, 32400,  "UTC+9 Tokyo"},       // I
      {31, 34200,  "UTC+9:30"},          // I+ "Adelaide" doesn't fit well.
      {32, 36000,  "UTC+10 Canberra"},   // K
      {33, 37800,  "UTC+10:30"},         // K+
      {34, 39600,  "UTC+11"},            // L
      {35, 41400,  "UTC+11:30"},         // L+
      {36, 43200,  "UTC+12 Auckland"},   // M 
      {37, 45900,  "UTC+12:45"},         // M+
      {38, 46800,  "UTC+13"},            // M+
      {39, 50400,  "UTC+14"},            // M+
    };
  
} // end timezone



long timeZoneOffset;
int daylightTimeOffset;

// timestamp: boot time
unsigned long bootTime;
// timestamp: time spent looking for a fix
unsigned long satSearchTime;

//LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
/*
  The circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 8
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 11
 * LCD D7 pin to digital pin 12
 * LCD R/W pin to ground
 * pot to LCD VO pin (pin 3)
*/
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

#define REDLCDLED 3
#define GREENLCDLED 5
#define BLUELCDLED 6

#define REDCOLOR 1
#define GREENCOLOR 2
#define BLUECOLOR 3

byte brightness = 255;
byte currentColor = BLUECOLOR;

TinyGPSPlus gps;
// There is an output sentence that will tell you the status of the antenna. $PGTOP,11,x where x is the status number.
// If x is 3 that means it is using the external antenna. If x is 2 it's using the internal antenna and
// if x is 1 there was an antenna short or problem.
TinyGPSCustom antenna(gps, "PGTOP", 2); // $PGTOP sentence, 2nd element

#define GPS_PA6H 1
#define PA6H_MESSAGES_DEFAULT "$PMTK314,-1*04"
#define PA6H_MESSAGES "$PMTK314,3,1,3,1,2,6,0,0,0,0,0,0,0,0,0,0,0,0,0*2C"
#define PA6H_ANTENNA_STATUS "$PGCMD,33,1*6C" 
static const uint32_t GPSBaud = 9600;


namespace TinyGpsPlusPlus {
  // This version of delay() from TinyGPS ensures that the gps object is being "fed".
  static void smartDelay(unsigned long ms) {
    unsigned long start = millis();
    do 
    {
      //while (ss.available()) //softwareserial
      //gps.encode(ss.read());  //softwareserial
      while (Serial.available())
        gps.encode(Serial.read());
    } while (millis() - start < ms);
  }
} // end namespace TinyGpsPlusPlus

namespace Adafruit {
  // https://learn.adafruit.com/character-lcds/rgb-backlit-lcds
  void setBacklight(uint8_t r, uint8_t g, uint8_t b) {
    // normalize the red LED - it's brighter than the rest!
    r = map(r, 0, 255, 0, 100);
    g = map(g, 0, 255, 0, 150);
    b = map(b, 0, 255, 0, 220); // this makes the blue less "deep" so the text is legible
 
    // brightness = 255 ; 
    r = map(r, 0, 255, 0, brightness);
    g = map(g, 0, 255, 0, brightness);
    b = map(b, 0, 255, 0, brightness);
 
    // common anode so invert!
    r = map(r, 0, 255, 255, 0);
    g = map(g, 0, 255, 255, 0);
    b = map(b, 0, 255, 255, 0);

    analogWrite(REDLCDLED, r);
    analogWrite(GREENLCDLED, g);
    analogWrite(BLUELCDLED, b);
  }
} // end namespace Adafruit

// Chronodot object
//Chronodot RTC;

// the first byte (address 0) of the EEPROM
// EEPROM addresses, 0 to 255
static byte eepromAddress0 = 0; // first run?
byte eepromDefaultsExist;
static byte eepromAddress1 = 1; // timezone
byte eepromTimeZone;
static byte eepromAddress2 = 2; // daylight time
byte eepromDaylightTime;
static byte eepromAddress3 = 3; // brightness
byte eepromBrightness;
static byte eepromAddress4 = 4; // 24 hour (0) or 12 hour (1)
byte eepromTimeFormat;
static byte eepromAddress5 = 5; // 2014-12-25 (0), 12/25/2014 (1), 25/12/2014 (2) 
byte eepromDateFormat;
static byte eepromAddress6 = 6; // slide left to right (0), bounce left or right (1)
byte eepromClockDisplayFormat;

byte statConfMenu = 0; // status: are we displaying configuration menu?

int switchConfMenu = A0;            // flip to enter Configuration menu
int switchDaylightTime = A1;        // flip to enable Daylight Time
int switchTwelveHour = A2;               // open for 12 hour time, jumpered for normal time.
int switchDebug = A3;
// A4 and A5 for i2c
byte potentiometerTimezone = 6;     //  input pin for the TimeZone potentiometer
byte potentiometerBrightness = 7;   // input pin for the Brightness potentiometer
#define ANTENNALED 4;
//int nmeaRx = 2;

const size_t sizeBuf = 17; // default size buffer for 16 character per line LCD

byte lastTimeZone = 99;
byte lastBrightness = 0;
byte lastDaylightTime = 99; // not zero or one
byte lastLoopSatsInView = 99;
byte showedConfMessage = 0;
byte inConfMode = 0;
byte antennaStatus = 3;
byte satellitesInView = 0;

// "A variable should be declared volatile whenever its value can be changed by
// something beyond the control of the code section in which it appears, such as
// a concurrently executing thread. In the Arduino, the only place that this is
// likely to occur is in sections of code associated with interrupts, called
// an interrupt service routine."

volatile byte pps_loop = 0;
void pps_interrupt(){
  // if we're not in the configuration menu and we have a good fix,
  if ( (!statConfMenu) && (gps.time.isValid()) )   {
    time_t currentEpoch;
    //convert GPS time to Epoch time
    currentEpoch =  epochConverter(gps.date, gps.time);
    // and display it on the LCD
    printTime(currentEpoch);
    // this counter is for information on the second line of the LCD.
    pps_loop++;
    if (pps_loop < 8) {
      // update the LCD with "Sats in View: 06" or similar
      printSats(satellitesInView); 
    } else if ( (pps_loop == 8) || (pps_loop == 9) ) {
      // updated LCD with year/month/day
      printDate(currentEpoch);
    } else if ( (pps_loop == 10) || (pps_loop == 11) ) {
      if (DEBUG) {
        displayUptime();
      }
    } else if ( pps_loop == 12) {
      if (DEBUG) {
        lcd.setCursor(0, 1);
        lcd.print(F("................"));
        updateLcdInt(1, freeRam());
      }
    } else  {
      pps_loop = 0;
    }
  } else  {
    // silently pass if we're in configuration mode or have no nmea data
    true; 
  }
}


time_t epochConverter(TinyGPSDate &d, TinyGPSTime &t) {
  // make the object we'll use
  tmElements_t tm;
  // if the TinyGPS time is invalid, we'll return past time to make it obvious.
  // Shouldn't really reach this point though because of the isValid check in pps_interrupt
  if (!t.isValid()) {
    return 0;
  } else {
    // GPS time is probably valid, let's construct seconds from it
    tm.Year =  CalendarYrToTm(d.year()); // TM offset is years from 1970, i.e. 2014 is 44.
    tm.Month =  d.month();
    tm.Day =  d.day();
    tm.Hour =  t.hour();
    tm.Minute =  t.minute();
    tm.Second =  t.second();
    time_t epochtime = makeTime (tm);
    setTime(epochtime);
    adjustTime(1);
    // we add one here because the time is from the NMEA sentence preceding the PPS signal
    return epochtime + 1  + timeZoneOffset + daylightTimeOffset;
  }
  // nothing matched? Shouldn't arrive here.
  return 1320001666;
}

void printDate(time_t currentEpoch) {
  // this is our time elements object,
  tmElements_t tm;
  // that we "break" into components, like Hour/Minute/Second
  breakTime(currentEpoch, tm);
  int dateFormat = getDateFormat() ;
  // because tm.Year is YYYY - 1970, and we want tmYY + 1970
  int year2k = tmYearToCalendar(tm.Year);
  char szBuf[sizeBuf+1];
  switch (dateFormat) {
    case 1:
      // 12/25/2014
      snprintf(szBuf, sizeBuf, "%02d-%02d-%04d      ", tm.Month, tm.Day, year2k);
      break;
    case 2:
      //25/12/2014
      snprintf(szBuf, sizeBuf, "%02d-%02d-%04d      ", tm.Day, tm.Month, year2k);
      break;
    default: 
      // 2014-12-25
      snprintf(szBuf, sizeBuf, "%04d-%02d-%02d      ", year2k, tm.Month, tm.Day);
  }
  // now push the formatted time to the screen.
  updateLcdText(1, szBuf);
}


// argument is "seconds since epoch" as created by epochConverter function.
void printTime(time_t currentEpochTime) {
  tmElements_t tm2;
  // that we "break" into components, like Hour/Minute/Second
  breakTime(currentEpochTime, tm2);
  // make the text string object
  char szBuf[sizeBuf+1];

  byte enable12 = getTimeFormat() ;
  byte adjustedHour;
  if (enable12) {
    // convert 24-hour to 12-hour format
    adjustedHour = ((tm2.Hour + 11) % 12 + 1);
  } else {
    adjustedHour = tm2.Hour;
  }

  if (getClockDisplayStyle() == 1) {
    // here we shift the clock for the top or bottom of the hour.          
    // you could switch it hourly with "tm.Hour % 2;"
    byte topOfTheHour = tm2.Minute < 30;
    if (topOfTheHour) {
      // if it's the top of the hour, display on the left.
      snprintf(szBuf, sizeBuf, "%02d:%02d:%02d        ", adjustedHour, tm2.Minute, tm2.Second);
    } else { 
      // if it's the bottom of the hour, display on the right.
      snprintf(szBuf, sizeBuf, "        %02d:%02d:%02d", adjustedHour, tm2.Minute, tm2.Second);
    }
  } else { // clockFormat default
    // slide clock left to right
    byte larsenShift = tm2.Minute / 7;
    // as far as I can tell, Arduino doesn't support variable width. So here's 200bytes of wasted RAM
    switch (larsenShift) {
      case (1):
        snprintf(szBuf, sizeBuf, " %02d:%02d:%02d       ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (2):
        snprintf(szBuf, sizeBuf, "  %02d:%02d:%02d      ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (3):
        snprintf(szBuf, sizeBuf, "   %02d:%02d:%02d     ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (4):
        snprintf(szBuf, sizeBuf, "    %02d:%02d:%02d    ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (5):
        snprintf(szBuf, sizeBuf, "     %02d:%02d:%02d   ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (6):
        snprintf(szBuf, sizeBuf, "      %02d:%02d:%02d  ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (7):
        snprintf(szBuf, sizeBuf, "       %02d:%02d:%02d ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      case (8):
        snprintf(szBuf, sizeBuf, "        %02d:%02d:%02d", adjustedHour, tm2.Minute, tm2.Second);
        break; 
      default:
        snprintf(szBuf, sizeBuf, "%02d:%02d:%02d        ", adjustedHour, tm2.Minute, tm2.Second);
        break; 
    } //endswitch
  }  //endif
  
  // now push the formatted time to the screen.
  updateLcdText(0, szBuf);
}

static void printSats(const int satellitesInView) {
  char szBuf[sizeBuf+1];
  snprintf(szBuf, sizeBuf, "Sats in View: %02d", satellitesInView);
  updateLcdText(1, szBuf);
}

void updateLcdText(const int row, const char message[17]){
  lcd.setCursor(0, row);
  //lcd.print("1..4...8..12..16);
  lcd.print(message);
}

void updateLcdInt(const byte row, const int number){
  lcd.setCursor(0, row);
  //lcd.print("1..4...8..12..16);
  lcd.print(number);
}

void fadeBacklight( int newColor, int oldColor = 0 ) {
  if ( newColor == REDCOLOR ) {
    for (int i = 0; i < 255; i++) {
    Adafruit::setBacklight(i, 255-i, 0);
    TinyGpsPlusPlus::smartDelay(5);}
  }
  if ( newColor == BLUECOLOR ) {
    for (int i = 0; i < 255; i++) {
    Adafruit::setBacklight(0, 255-(.5 * i ), i); // this blue is 0, 127,255 because without the green it's a deep deep blue
    TinyGpsPlusPlus::smartDelay(5); }
  }
  if ( ( newColor == GREENCOLOR ) && (oldColor == REDCOLOR ) ) {
    for (int i = 0; i < 255; i++) {
    Adafruit::setBacklight(255-i, i, 0);
    TinyGpsPlusPlus::smartDelay(5); }
  }
  if ( ( newColor == GREENCOLOR ) && (oldColor == BLUECOLOR ) ) {
    for (int i = 0; i < 255; i++) {
    Adafruit::setBacklight(0, (127 + (.5 * i)), 255-i); // blue is already at 127 on the green
    TinyGpsPlusPlus::smartDelay(5); }
  }
  currentColor = newColor; // used in changeBacklightColor to see if any updates need to happen
}

void changeBacklightColor(const int newColor = BLUECOLOR) {
  // here we establish the order the colors fade: from red to green to blue or vice versa.
  // This might be in one step, red to green, or multiple steps, blue to red via green.
  
#warning fixme
  // what does this do? leftover from yellow?
  if (currentColor >= 4) {
    currentColor = 0;
  }
  
  if (currentColor == newColor) {
    // nothing to do with the backlight, it's already set correctly
    true;
  } else if ( (abs(currentColor - newColor) == 2)) {
    //stepping from red to blue or blue to red via green
    switch (currentColor) {
    case BLUECOLOR:
      //  to green from blue
      fadeBacklight( GREENCOLOR, BLUECOLOR );
      // to red from green
      fadeBacklight( REDCOLOR, GREENCOLOR );
      break;
    case REDCOLOR:
      // to green from red
      fadeBacklight( GREENCOLOR, REDCOLOR );
      // to blue from green
      fadeBacklight( BLUECOLOR, GREENCOLOR );
      break;
    } // end switch
  } else if ( (abs(currentColor - newColor) == 1)) {
    //stepping from red to green or blue to green 
    switch (currentColor) {
    case BLUECOLOR:
      // to green from blue
      fadeBacklight( GREENCOLOR, BLUECOLOR );
      break;
    case REDCOLOR:
      // to green from red
      fadeBacklight( GREENCOLOR, REDCOLOR );
      break;
    case GREENCOLOR: 
      // to red or blue from green
      fadeBacklight( newColor, GREENCOLOR );
    } // end switch
  } else { 
    // no matches
    true;
  } // endif
}

int readPotentiometer(const int thisPot) {
  // read the value of the potentiometer (0 to 1023) 
  return analogRead(thisPot);
}

int readSwitch(const int thisSwitch) {
  // one is open, zero is closed.
  int valueSwitch = digitalRead(thisSwitch);
  valueSwitch = constrain(valueSwitch, 0, 1);
  return valueSwitch;
}

int readTimeZone() {
  int valueTimeZone = readPotentiometer(potentiometerTimezone);
  // http://www.jetmore.org/john/blog/2011/09/arduinos-map-function-and-numeric-distribution/
  // plus one the max values to deal with unequal distribution
  valueTimeZone = map(valueTimeZone, 0, 1024, 0, 40 );
  valueTimeZone = constrain(valueTimeZone, 0, 39);
  return valueTimeZone;
}

int readBrightness() {
  int valueBrightness = readPotentiometer(potentiometerBrightness);
  valueBrightness = map(valueBrightness, 0, 1024, 0, 256);     // scale it to use it with rgb (value between 0 and 255) 
  valueBrightness = constrain(valueBrightness, 0, 255);
  return valueBrightness;
}

void saveSettings(const int eepromAddress, const int newValue) {
  int eepromValue = EEPROM.read(eepromAddress);
  // only write if new value and saved value are different
  if  (eepromValue != newValue ) {
    EEPROM.write(eepromAddress, newValue);
  }
}

byte getTimeZone() {
 return EEPROM.read(eepromAddress1);
}

byte getDaylightTime() {
  return EEPROM.read(eepromAddress2);
}

byte getBrightness() {
 return EEPROM.read(eepromAddress3);
}

byte getTimeFormat() {
 return EEPROM.read(eepromAddress4);
}

byte getDateFormat() {
 return EEPROM.read(eepromAddress5);
}

byte getClockDisplayStyle() {
 return EEPROM.read(eepromAddress6);
}

long getTimeZoneOffset() {
  eepromTimeZone = getTimeZone();
  return  TimeZoneList::tzlist[eepromTimeZone].tzoffset;;
}

int getDaylightTimeOffset() {
  eepromDaylightTime = getDaylightTime();
  if (eepromDaylightTime) {
    return 3600;
  }
  return 0;
}

void displayUptime() {
  // time since boot in milliseconds
  unsigned long currentTime = millis();
  // subtract saved boot time. we could just assume that's zero. convert to seconds.
  currentTime = ((currentTime - bootTime) / 1000) ;

  byte seconds = (currentTime)%60;
  byte minutes = ((currentTime)%3600)/60;
  byte hours = ((currentTime)%86400)/3600;
  //byte days = ((currentTime)%2592000)/86400;
  byte days = floor(currentTime / (60 * 60 * 24) ); 

  char szBuf[sizeBuf+1];
  // rolls over around 50 days.
  snprintf(szBuf, sizeBuf, "Up:  %02d,%02d:%02d:%02d", days, hours, minutes, seconds);
  //snprintf(szBuf, sizeBuf, "Uptime: %02d:%02d:%02d", hours, minutes, seconds);
  updateLcdText( 1,szBuf); 
}

void eepromCreateDefaults() {
  eepromDefaultsExist = 1; //anything but zero
  eepromTimeZone = 8 ; // number from array of defined timezones
  eepromDaylightTime = 0; // "no" is a fine default
  eepromTimeFormat = 1; // 23:11:59
  eepromDateFormat = 0; // 2014-12-25
  EEPROM.write(eepromAddress0, eepromDefaultsExist);
  EEPROM.write(eepromAddress1, eepromTimeZone);
  EEPROM.write(eepromAddress2, eepromDaylightTime);
  EEPROM.write(eepromAddress4, eepromTimeFormat);
  EEPROM.write(eepromAddress5, eepromDateFormat);
}

void clearScreen() {
  lcd.setCursor(0, 0);
  lcd.print(F("                "));
  lcd.setCursor(0, 1);
  lcd.print(F("                "));
}

void displayBootScreen() {
  lcd.setCursor(0, 0);
  lcd.print(F(JEMMA_VERSION));
  lcd.setCursor(0, 1);
  lcd.print(F(JEMMA_COPYRIGHT));
  delay(1500);
}

void configurationMenu() {
  //detachInterrupt(0); // we could detach, but.. we'll just not update the screen with pps right now.
  if (!showedConfMessage) { // inConfMode
    updateLcdText( 0,"Configuration   ");
    updateLcdText( 1, "                ");
    showedConfMessage = 1;
    changeBacklightColor(REDCOLOR); // set the LCD to red.
  }
  
  brightness = readBrightness();
  if ( lastBrightness != brightness) {
    // we're using the setBacklightFunction to refresh the brightness as it changes.
    Adafruit::setBacklight(255, 0, 0);
  }

  // read the current time zone from the pot
  int statTimeZone = readTimeZone();
  // if the remembered value is different from the current value,
  if ( lastTimeZone != statTimeZone ) {
    // update the screen to reflect changes
    lcd.setCursor(0, 1);
    lcd.print(F("                "));
    //updateLcdText( 1, "                ");
    char *displayZone = TimeZoneList::tzlist[statTimeZone].tzname;
    updateLcdText( 1, displayZone);
    // remember the current value so we can skip the screen update next loop
    lastTimeZone = statTimeZone;
  } else { 
    true;
  }

  // read the current DST setting from the switch.  
  int statDaylightTime = readSwitch(switchDaylightTime) ;
  // if the value from the last run does not match the current setting, 
  if (lastDaylightTime != statDaylightTime) {
    // update the LCD to reflect new setting
    lcd.setCursor(0, 0);
    if (statDaylightTime) {
      lcd.print(F("DST Enabled     "));
    } else {         
      lcd.print(F("DST Disabled    "));
    }
    // remember the current value so LCD update can be skipped
    lastDaylightTime = statDaylightTime;
  }
}

void setup() {
  // eight second watchdog timer
  wdt_enable(WDTO_8S);

  // configure pins
  // 2 is NMEA Rx
  pinMode(2, INPUT);
  // pin 13, used for antenna LED
  pinMode(LED_BUILTIN, OUTPUT);
  // pin 4, used for antenna LED
  pinMode(4, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // pins for RGB LCD colors
  pinMode(REDLCDLED, OUTPUT);
  pinMode(GREENLCDLED, OUTPUT);
  pinMode(BLUELCDLED, OUTPUT);

  // switches
  pinMode(switchConfMenu, INPUT);
  digitalWrite(switchConfMenu, HIGH); // set pullup
  pinMode(switchDaylightTime, INPUT);
  digitalWrite(switchDaylightTime, HIGH); // set pullup
  pinMode(switchTwelveHour, INPUT);
  digitalWrite(switchTwelveHour, HIGH); // set pullup

  // this RGB LCD is 16 columns, 2 rows
  lcd.begin(16, 2);
  // boot backlight is blue, "get" is from eeprom
  brightness = getBrightness() ;
  Adafruit::setBacklight(0, 127, 255);
  // Print bootup screen to LCD.
  displayBootScreen();

  //#ifdef SOFTSERIAL
  //  ss.begin(GPSBaud);  //softwareserial
  //  ss.println(PGCMD_ANTENNA_STATUS);  //softwareserial
  //#else

  Serial.begin(GPSBaud); 
  if (GPS_PA6H) {
    Serial.println(PA6H_MESSAGES_DEFAULT);
    delay(100);
    Serial.println(PA6H_MESSAGES);
    delay(100);
    Serial.println(PA6H_ANTENNA_STATUS);  //softwareserial
  }

  // check to see if this is the first run
  eepromDefaultsExist = EEPROM.read(eepromAddress0);
  // if it is the first run, create defaults
  if (!eepromDefaultsExist) {
     eepromCreateDefaults;
  }

  // fetch the timezone and daylight time settings
  daylightTimeOffset = getDaylightTimeOffset() ;
  timeZoneOffset = getTimeZoneOffset() ;

  //byte enable12 = getTimeFormat();
  byte enable12 = readSwitch(switchTwelveHour);
  saveSettings(eepromAddress4, enable12);

  // long enough to get a nmea sentence maybe
  TinyGpsPlusPlus::smartDelay(1000); 

  // force the screen blank
  clearScreen();

  // counter for uptime and satSearch
  bootTime = millis();
  satSearchTime = millis();
  
  // begin listening for PPS
  attachInterrupt(0, pps_interrupt, RISING);
}

void loop(){
  // strtonum the custom PGTOP nmea message for antenna status, internal, short, or active
  antennaStatus = atoi(antenna.value());

  // this section is about setting configuration items.
  statConfMenu = readSwitch(switchConfMenu) ;
  
  // if we go into config mode, everything else gets paused effectively. no nmea read, etc.
  if (statConfMenu) {
    // begin confMenu
    inConfMode = 1;
    configurationMenu();
  } else { // end conf screen, else is the "normal" situation 
    if (inConfMode) { // clear display if we just left confMode
      // save the new settings
      saveSettings(eepromAddress1, lastTimeZone);
      saveSettings(eepromAddress2, lastDaylightTime);
      saveSettings(eepromAddress3, brightness);
      // clear the display
      clearScreen();
      // reset some cached values
      showedConfMessage = 0; 
      lastTimeZone = 99;
      lastDaylightTime = 99;
      // force screen update
      lastLoopSatsInView = 99 ;
      // pick up the changed settings
      daylightTimeOffset = getDaylightTimeOffset() ;
      timeZoneOffset = getTimeZoneOffset() ;
      // finally reset what brought us here
      inConfMode = 0;
    }
    // first set the backlight color based on number of satellites
    if (gps.satellites.isValid()) { // true even with zero satellites, as in, true with NMEA sentences coming in
      satellitesInView = gps.satellites.value();
      if (lastLoopSatsInView != satellitesInView) { // cache
        if ( satellitesInView <= 1 ) { // if there are three or less satellites,
          changeBacklightColor(REDCOLOR); // set the LCD to red.
        } else if ( (satellitesInView == 2) || (satellitesInView == 3) ) { // if there are four or five satellites,
          changeBacklightColor(REDCOLOR); // set the LCD to red.
        } else if ( (satellitesInView == 4) || (satellitesInView == 5) ) { // if there are four or five satellites,
          changeBacklightColor(GREENCOLOR);  // set the LCD to green
        } else if ( satellitesInView >= 6 ) { // if there are six or more satellites,
          changeBacklightColor(BLUECOLOR); // set the LCD to blue
        } else { // if none of those matched for some weird reason
          changeBacklightColor(REDCOLOR); // set the LCD to red.
        }
      }
      // save the current count so we can skip the loop next time
      lastLoopSatsInView = satellitesInView;

      // this section updates the LCD if there isn't PPS.
      // otherwise, LCD updates happen from PPS so there aren't locking problems.
      if ( (satellitesInView == 2) || (satellitesInView == 3) ) {
         time_t currentEpochNotPps =  epochConverter(gps.date, gps.time);
         printTime(currentEpochNotPps);  // since it's not coming from pps yet, but we have time...
         printSats(satellitesInView);  // update the LCD with "Sats in View: 06" or similar
      } //endif sats=2,3
      
      // reset the satSearch counter for if we later lose sats after a fix
      if (satellitesInView > 1) {
               satSearchTime = millis();
      } //endif sats>1
      
      if (satellitesInView <= 1) {
        unsigned long currentTime = millis();
        currentTime = currentTime - satSearchTime ;
        byte seconds = (currentTime/1000)%60;
        byte minutes = (currentTime/1000/60)%60;
        byte hours = ((currentTime/1000)%86400)/3600;

        char szBuf[sizeBuf+1];
        if (seconds < 30) {
            lcd.setCursor(0, 0);
            lcd.print(F("                "));
            lcd.setCursor(0, 1);
            lcd.print(F("Satellite Search"));
            snprintf(szBuf, sizeBuf, "%02d:%02d:%02d Elapsed", hours, minutes, seconds);
          updateLcdText( 0,szBuf); 
        } else {
            lcd.setCursor(0, 0);
            lcd.print(F("Satellite Search"));
            lcd.setCursor(0, 1);
            lcd.print(F("                "));

          snprintf(szBuf, sizeBuf, "%02d:%02d:%02d Elapsed",hours,  minutes, seconds);
          updateLcdText( 1,szBuf);
        } //endif top/bottom swap
      } //endif sats <=1

    } else {
      // in case isValid didn't match and there's no pps_interrupt happening,
      changeBacklightColor(REDCOLOR); // set the LCD to red.
      lcd.setCursor(0, 1); // column zero, row zero
      lcd.print(F("GPS Problem.    "));
    }

 
    // this will show an error at bootup if things look totally broke. 
    if (millis() > 5000 && gps.charsProcessed() < 10) {
        updateLcdText( 0, "                ");
        updateLcdText( 1, "GPS Rx Failure  ");
        TinyGpsPlusPlus::smartDelay(2000);               // wait for two seconds
     }

    // pause main loop for a third of a second
    TinyGpsPlusPlus::smartDelay(353);               

    // update LED. Off with connected antenna, 3 
    if (antennaStatus == 3) {
      digitalWrite(LED_BUILTIN, LOW);
      digitalWrite(4, LOW);
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(4, HIGH);
    } //endif antenna status
  }

// check for watchdog timer reset
wdt_reset();

}

