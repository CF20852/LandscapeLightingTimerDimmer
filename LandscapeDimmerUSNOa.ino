/*
 * LandscapeDimmer.ino
 * Turns on low-voltage landscape lights starting at sunset + DELAY_AFTER_SUNSET
 * Turns off the lights starting at a fixed time, currently NINE_THIRTY_PM
 */
 
#include <Ethernet.h>
#include <DNS.h>
#include <EthernetUdp.h>
#include <PWM.h>
#include <SPI.h>
#include <TimeLib.h>
#include <Wire.h>
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

#define DEBUG                 1        // debug on
#undef  DEBUG                          // debug off

#define USE_SUNSET_TABLE      1        // use the table of sunset times
#undef  USE_SUNSET_TABLE               // or use the function instead

#define LAT                  34.58     // degrees
#define LON                -112.445    // degrees

#define INITIAL_LIGHT_LEVEL  15.0      // percent
#define FINAL_LIGHT_LEVEL    62.5      // percent
#define LIGHTS_OFF            0.0      // percent
#define DELAY_AFTER_SUNSET 1200        // seconds
//#define DELAY_AFTER_SUNSET    0        // seconds, set this for testing
#define BRIGHTEN_INTERVAL   300        // seconds
#define DIM_INTERVAL        300        // seconds
#define NINE_THIRTY_PM    77400        // seconds
#define MIDNITE               0        // seconds
//#define NINE_THIRTY_PM    69660        // seconds, set this for testing

#define D2R                  0.0174533 // degrees to radians multiplier
#define R2D                 57.2957795 // radians to degrees multiplier
#define ZENITH              90.8333    // Sun angle at zenith
#define TIMEZONE            -7.0       // Arizona time

#define FET_GATE             9         // Which PWM output pin to use
#define PWM_FREQUENCY      128         // Hz - keep it fast enough for the eye and slow to mitigate RF noise

#define GAMMA                3.0       // constant for gamma correction, not your father's mother

time_t prevDisplay = 0; // when the digital clock was displayed

#ifdef USE_SUNSET_TABLE
// This is a table of sunset times at my house in seconds after midnight for the year 2024 (a leap year)
const long sunsetSecsAfterMidnite[] PROGMEM = 
  {62975, 63020, 63066, 63113, 63162, 63211, 63262, 63313, 63366, 63419, 63473, 63528, 63583,
   63639, 63696, 63753, 63810, 63869, 63927, 63986, 64045, 64105, 64165, 64225, 64285, 64345, 64405,
   64466, 64526, 64587, 64647, 64708, 64768, 64828, 64888, 64948, 65008, 65067, 65126, 65185, 65244,
   65302, 65361, 65419, 65476, 65533, 65590, 65647, 65703, 65759, 65815, 65870, 65925, 65979, 66034,
   66087, 66141, 66194, 66247, 66299, 66352, 66403, 66455, 66506, 66557, 66608, 66658, 66708, 66758,
   66808, 66857, 66906, 66955, 67004, 67052, 67100, 67148, 67196, 67244, 67292, 67339, 67387, 67434,
   67481, 67528, 67575, 67622, 67669, 67716, 67763, 67810, 67856, 67903, 67950, 67997, 68044, 68090,
   68137, 68184, 68231, 68278, 68325, 68372, 68419, 68467, 68514, 68561, 68609, 68656, 68704, 68751,
   68799, 68847, 68895, 68942, 68990, 69038, 69086, 69134, 69182, 69230, 69278, 69326, 69374, 69422,
   69470, 69517, 69565, 69613, 69660, 69707, 69754, 69801, 69848, 69894, 69940, 69986, 70031, 70076,
   70121, 70165, 70209, 70252, 70295, 70337, 70379, 70420, 70460, 70500, 70538, 70577, 70614, 70651,
   70686, 70721, 70755, 70788, 70820, 70850, 70880, 70909, 70936, 70963, 70988, 71011, 71034, 71055,
   71075, 71094, 71111, 71127, 71141, 71154, 71165, 71175, 71183, 71190, 71195, 71199, 71201, 71201,
   71200, 71197, 71193, 71186, 71179, 71169, 71158, 71145, 71131, 71114, 71097, 71077, 71056, 71033,
   71008, 70982, 70955, 70925, 70894, 70862, 70827, 70792, 70754, 70715, 70675, 70633, 70590, 70545,
   70498, 70451, 70402, 70351, 70299, 70246, 70191, 70136, 70079, 70020, 69961, 69900, 69838, 69775,
   69711, 69646, 69579, 69512, 69444, 69375, 69304, 69233, 69161, 69088, 69015, 68940, 68865, 68789,
   68712, 68635, 68557, 68478, 68398, 68319, 68238, 68157, 68076, 67994, 67911, 67828, 67745, 67662,
   67578, 67493, 67409, 67324, 67239, 67154, 67069, 66984, 66898, 66812, 66727, 66641, 66555, 66470,
   66384, 66299, 66213, 66128, 66043, 65958, 65874, 65789, 65705, 65622, 65538, 65455, 65373, 65291,
   65209, 65128, 65047, 64967, 64888, 64809, 64731, 64653, 64577, 64501, 64426, 64351, 64278, 64205,
   64133, 64063, 63993, 63924, 63856, 63790, 63724, 63660, 63596, 63534, 63474, 63414, 63356, 63299,
   63243, 63189, 63137, 63085, 63036, 62987, 62940, 62895, 62852, 62810, 62769, 62731, 62694, 62658,
   62625, 62593, 62563, 62535, 62508, 62483, 62461, 62440, 62421, 62404, 62388, 62375, 62363, 62354,
   62346, 62341, 62337, 62335, 62335, 62337, 62341, 62347, 62354, 62364, 62375, 62389, 62404, 62421,
   62440, 62460, 62483, 62507, 62533, 62560, 62589, 62620, 62652, 62686, 62722, 62759, 62797, 62837,
   62878, 62920, 62964};
  
// This table gives the day of the year - 1 for the first day of each month:
const int daysToMonth[2][12] PROGMEM =
    // non-leap year
    { { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
    // leap year
      { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 } };

#endif

int dayOfYear;
long currentTimeSeconds, sunset, offTime;
float lightLevel, lightLevelIncr;

enum dimmerStates {
  WAITING_FOR_SUNSET,
  TURNING_UP,
  WAITING_FOR_OFFTIME,
  TURNING_DOWN,
  WAITING_FOR_MIDNITE
};
enum dimmerStates dimmerState;

byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x2D, 0x50 };

// NTP Servers--a DNS call to time.nist.gov should result in NIST's selected server IP addr:
IPAddress timeServer(132, 163, 97, 1); // time-a-wwv.nist.gov
// IPAddress timeServer(132, 163, 97, 2); // time-b-wwv.nist.gov
// IPAddress timeServer(132, 163, 97, 3); // time-c-wwv.nist.gov

const int timeZone = -7;     // Arizona Time
//const int timeZone = -5;     // Eastern Standard Time (USA)
//const int timeZone = -4;     // Eastern Daylight Time (USA)
//const int timeZone = -8;     // Pacific Standard Time (USA)
//const int timeZone = -7;     // Pacific Daylight Time (USA)

EthernetUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

void setup() 
{
  //#ifdef DEBUG
  Serial.begin(9600);
  while (!Serial) ; // Needed for Leonardo only
  delay(250);
  //#endif
  
  //initialize all timers except for 0, to save time keeping functions
  InitTimersSafe();

  //sets the frequency for the specified high-power FET gate driver pin
  bool success = SetPinFrequencySafe(FET_GATE, PWM_FREQUENCY);

  //if the pin frequency was not set, send a message to operator
  #ifdef DEBUG
  if (!success) {
    Serial.println("Failed to set frequency!");
  }
  #endif

  //set the CPU-based clock to reference clock sync interval to 10 minutes
  setSyncInterval(600);

  #ifdef DEBUG
  Serial.println("Ethernet initialization...");
  #endif
  
  if (Ethernet.begin(mac) == 0) {
    // no point in carrying on, so do nothing forevermore:
    while (1) {
      #ifdef DEBUG
      Serial.println("Failed to configure Ethernet using DHCP");
      delay(10000);
      #endif
    }
  }

  #ifdef DEBUG
  Serial.print("Board IP number assigned by DHCP is ");
  Serial.println(Ethernet.localIP());
  #endif
  
  Udp.begin(localPort);

  /*
   * The following code attempts to get NIST to assign a Time Server IP address
   */

  setDS1307();
}

void loop()
{  
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display and dimmer state only if time has changed
      prevDisplay = now();
      lightLevel = updateDimmerState();
      
      #ifdef DEBUG
      digitalClockDisplay();
      #endif
    }
  }
}

// This function provides a state machine to sequence the brightening and dimming of the lights.
float updateDimmerState() {
  int currentYear, currentMonth, currentDay, currentHour, currentMinute, currentSecond;

  currentYear = year();
  currentMonth = month();
  currentDay = day();
  
  currentHour = hour();
  currentMinute = minute();
  currentSecond = second();
  currentTimeSeconds = (long)currentHour * 3600L + (long)currentMinute * 60L + (long)currentSecond;

  //#ifdef DEBUG
  Serial.print("Current time in seconds = ");
  Serial.println(currentTimeSeconds);
  
  Serial.print("Dimmer state = ");
  Serial.println(dimmerState);
  //#endif
  
  switch (dimmerState) {

  //the first case is when we're sometime between midnite and sunset, waiting for sunset to occur
  case WAITING_FOR_SUNSET:
    #ifdef USE_SUNSET_TABLE
    dayOfYear = getDayOfYear(now());
    #endif
    
    #ifdef DEBUG
    Serial.print("DayOfYear = ");
    Serial.println(dayOfYear);
    #endif

    #ifdef USE_SUNSET_TABLE
    sunset = pgm_read_dword(&(sunsetSecsAfterMidnite[dayOfYear]));
    //sunset = 68880L; //test
    #endif

    #ifndef USE_SUNSET_TABLE
    sunset = calculateSunset(currentYear, currentMonth, currentDay, LAT, LON);
    #endif
    
    //#ifdef DEBUG
    Serial.print("Sunset = ");
    Serial.println(sunset);
    //#endif

    offTime = NINE_THIRTY_PM;
    if ( (currentTimeSeconds > sunset + DELAY_AFTER_SUNSET) && (currentTimeSeconds < offTime) ) {
      dimmerState = TURNING_UP;
      lightLevel = INITIAL_LIGHT_LEVEL;
    }
    else {
      lightLevel = LIGHTS_OFF;
      setLightLevel(lightLevel);
    }
    break;

  // The next case is for the interval during which we're turning up the lights.
  case TURNING_UP:
    lightLevelIncr = (FINAL_LIGHT_LEVEL - INITIAL_LIGHT_LEVEL) / BRIGHTEN_INTERVAL;
    lightLevel = lightLevel + lightLevelIncr;
    setLightLevel(lightLevel);
    
    if (currentTimeSeconds > sunset + DELAY_AFTER_SUNSET + BRIGHTEN_INTERVAL) {
      dimmerState = WAITING_FOR_OFFTIME;
      offTime = NINE_THIRTY_PM;
    }
    break;  

  // This caseis for the interval during which we're waiting until time to turn down the lights.
  case WAITING_FOR_OFFTIME:
    lightLevel = FINAL_LIGHT_LEVEL;
    setLightLevel(lightLevel);
    
    if (currentTimeSeconds > offTime) {
      dimmerState = TURNING_DOWN;
    }
    break;  

  // This case is for when we turn down the lights over some specified interval.
  case TURNING_DOWN:
    lightLevelIncr = (FINAL_LIGHT_LEVEL - INITIAL_LIGHT_LEVEL) / DIM_INTERVAL;
    lightLevel = lightLevel - lightLevelIncr;
    setLightLevel(lightLevel);
        
    if (currentTimeSeconds > offTime + DIM_INTERVAL) {
      dimmerState = WAITING_FOR_MIDNITE;
      lightLevel = LIGHTS_OFF;
      setLightLevel(lightLevel);
    }
    break;

  // This case is for the situation where the lights are down and we're waiting
  // for the day to roll over, e.g., moving from 23:59:59 to 00:00:00 (-0, + 5 minutes)
  case WAITING_FOR_MIDNITE:
    lightLevel = LIGHTS_OFF;
    setLightLevel(lightLevel);

    // Trigger a state change if the time is anywhere between 0:00:00 and 0:05:00
    // in case the clock gets updated and suddenly jumps a few minutes
    if (currentTimeSeconds < MIDNITE + 300) {
      setDS1307();
      dimmerState = WAITING_FOR_SUNSET;
    }
    break;

  // Oops, nothing matches.  Let's assume we're sometime during the day before sunset.
  default:
    dimmerState = WAITING_FOR_SUNSET;
    break;
  }

  return lightLevel;
}

// This function prints formatted time of day in HH:MM:SS format and date in YYYY MM DD format.
void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(year()); 
  Serial.print(" ");
  Serial.print(month());
  Serial.print(" ");
  Serial.print(day());
  Serial.println();
}

/* 
 * This function is used to format and print the hours and minutes part of a time of day,
 * which can be in the range of 0-9 and need a leading 0, or in the range 10-59 and not
 * need a leading 0.
 */
void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------
 * This function sends a NTP request to the selected time server and parses the result.
 */

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets

  #ifdef DEBUG
  Serial.println("Transmit NTP Request");
  #endif
  
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      #ifdef DEBUG
      Serial.println("Receive NTP Response");
      #endif
      
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }

  #ifdef DEBUG
  Serial.println("No NTP Response :-(");
  #endif
  
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// This function retrieves time from an NTP time server and uses it to set the DS1307 RTC
void setDS1307() {
  int ret = 0;
  IPAddress remote_addr;    
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());

  // time.nist.gov is the general NIST timeserver address.  A DNS query to this url
  // will return the IP address of a time server assigned by NIST.
  ret = dns.getHostByName("time.nist.gov", remote_addr);
  if (ret == 1) {

      #ifdef DEBUG
      Serial.print("Time Server IP: ");
      Serial.println(remote_addr);
      #endif
      
      timeServer = remote_addr;
  }
  else
  {
    #ifdef DEBUG
    Serial.println("getHostByName failed to get Time Server IP address using time.nist.gov");
    Serial.print("ret = ");
    Serial.println(ret);
    #endif
    
  }
  /* end of code to get NIST to assign Time Server IP address */

  #ifdef DEBUG
  Serial.println("waiting for sync");
  #endif
  
  setSyncProvider(getNtpTime);
  
  /* 
   *  The following three lines of code set the DS1307 to time retrieved from the NTP server
   *  and switch the time sync provider over to the DS1307.  Warning:  No error checking performed.
   */
  RTC.set(now());   // set the RTC and the system time to the received value
  setTime(now()); 
  setSyncProvider(RTC.get);   // the function to get the time from the RTC
}

#ifdef USE_SUNSET_TABLE
// This function checks to see if the current year is a leap year.
bool isLeap(int const year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// This function gets the day of the year using the table of first days of each month.
int getDayOfYear(time_t t) {
  //int ly = isLeap(year(t));
  int ly = 1;  //because the sunsetTime table is for 2024, assume leap year

  return(pgm_read_word(&(daysToMonth[ly][month(t) - 1])) + day(t) - 1);
}
#endif

// This function sets the duty cycle of the PWM output that drives the high-current switch gate.
void setLightLevel(float level) {
  // the argument 'level' is a percentage, from 0 (off) to 100 (full on)
  uint16_t gammaCorrectedLevel;
  
  gammaCorrectedLevel = gammaCorrection(level);
  pwmWriteHR(FET_GATE, gammaCorrectedLevel);  //use this functions instead of analogWrite on 'initialized' pins

  #ifdef DEBUG
  Serial.print("Level = ");
  Serial.println(gammaCorrectedLevel);
  #endif
  
}

/*
 * This function calculates a 16-bit unsigned int corresponding to the percentage value for the desired light level,
 * using the gamma correction function where 'arg' is the base and gamma is the exponent.  The result is constrained to
 * a range of 1-65535.
 */
uint16_t gammaCorrection(float arg) {
  //Perform a gamma correction.  LEDs do not have a linear brightness perceived by human vision.  This helps to simulate that.
  //Pass in a value between 0 and Max as a floating pt percentage, and a gamma corrected value will be returned as an unsigned int
  float scaleFactor, gammaCorrectedValue;
  scaleFactor = 65535.0 / pow(100, GAMMA);
  gammaCorrectedValue = constrain( (pow(arg, GAMMA) * scaleFactor), 1.0, 65535.0);
  return (uint16_t) gammaCorrectedValue;
  
}

/*
 * This function calculates the time of sunset in seconds past midnight.  It is based
 * on an algorithm in the United States Naval Observatory's Almanac for Computers, 1990.
 * It can be modified to calculate the time of sunrise by changing two lines of code,
 * as indicated in the comments below.  Also, "sunset" would be changed to "sunrise"
 * in several places.
 */
long calculateSunset(int year, int month, int day, double lat, double lon) {
  float latr, lonr;
  int N, N1, N2, N3;
  int Lquad, RAquad;
  long sunset;
  // note that double is the same as float on AVR-based Arduinos such as the Uno, but floats seem good enough
  double lambda, t, M, L, tanRA, RA, delta, sinDec, cosDec, cosH, H, T, UT, LT;
  
  latr = lat * D2R;  // latitude in radians
  lonr = lon * D2R;  // longitude in radians

  // calculate the day of the current year, N.  N1, N2, and N3 are intermediate calculations.
  N1 = 275 * month / 9;
  N2 = (month + 9) / 12;
  N3 = 1 + (year - 4 * (year / 4) + 2) / 3;

  N = N1 - (N2 * N3) + day - 30;

  #ifdef DEBUG
  Serial.print("N1, N2, N3, N = ");
  Serial.print(N1);
  Serial.print(", ");
  Serial.print(N2);
  Serial.print(", ");  
  Serial.print(N3);
  Serial.print(", "); 
  Serial.print(N);
  Serial.println();
  #endif

  // convert longitude to hours, remembering the Earth rotates 15 degrees per hour
  lambda = lon / 15.0;
  #ifdef DEBUG
  Serial.print("lambda = ");
  Serial.println(lambda, 6);
  #endif

  // calculate a rough estimate of sunset time, t
  t = (double) N + (18.0 - lambda) / 24.0;  // for calculating sunset time
  //t = (double) N + (6.0 - lambda) / 24.0;   // for calculating sunrise time
  #ifdef DEBUG
  Serial.print("t = ");
  Serial.println(t, 6);
  #endif

  // calculate the mean anomaly of the Sun, M
  M = 0.9856 * t - 3.289;
  M = fmod(M, 360.0);
  #ifdef DEBUG
  Serial.print("M = ");
  Serial.println(M, 6);
  #endif
  
  // calculate the Sun's true longitude, L
  L = M + 1.916 * sin(M * D2R) + 0.02 * sin(2 * M * D2R) + 282.634;
  L = fmod(L, 360.0);
  #ifdef DEBUG
  Serial.print("L = ");
  Serial.println(L, 6);
  #endif

  // calculate the Right Ascension (RA) of the Sun
  tanRA = 0.91746 * tan(L * D2R);
  RA = atan(tanRA) * R2D;

  // ensure RA is in the range 0-359.999999 degrees
  while (RA < 0) {
    RA += 360.0;
  }

  while (RA >= 360.0) {
    RA -= 360.0;
  }

  #ifdef DEBUG
  Serial.print("RA = ");
  Serial.println(RA, 6);
  #endif

  Lquad = (int)(L / 90) * 90;
  RAquad = (int)(RA / 90) * 90;

  #ifdef DEBUG
  Serial.print("Lquad = ");
  Serial.println(Lquad);
  #endif

  #ifdef DEBUG
  Serial.print("RAquad = ");
  Serial.println(RAquad);
  #endif

  // ensure RA is in the same quadrant as L and convert to hours
  RA = RA + (double)(Lquad - RAquad);
  RA = RA / 15.0;

  #ifdef DEBUG
  Serial.print("RAhrs = ");
  Serial.println(RA, 6);
  #endif

  // calculate the sine and cosine of the Sun's declination
  sinDec = 0.39782 * sin(L * D2R);
  cosDec = sqrt(1 - sinDec * sinDec);

  #ifdef DEBUG
  Serial.print("sinDec = ");
  Serial.println(sinDec, 6);
  #endif

  // calculate the Sun's local hour angle, H
  cosH = (cos(ZENITH * D2R) - sinDec * sin(latr)) / (cosDec * cos(latr));

  #ifdef DEBUG
  Serial.print("cosH = ");
  Serial.println(cosH, 6);
  #endif

  H = acos(cosH) * R2D / 15.0;  // for calculating sunset time
  //H = ( 360.0 - acos(cosH) ) * R2D / 15.0;  // for calculating sunrise time

  #ifdef DEBUG
  Serial.print("H = ");
  Serial.println(H, 6);
  #endif

  // calculate the local mean time of sunset, T
  T = (H + RA - 0.06571 * t - 6.622);

  // ensure T is in the range 0-23.999999 hours
  while (T < 0) {
    T = T + 24.0;
  }
  while (T >= 24.0) {
    T = T - 24.0;
  }

  #ifdef DEBUG
  Serial.print("T = ");
  Serial.println(T, 6);
  #endif

  // calculate the Universal Coordinated Time of local sunset, UT
  UT = T - lambda;

  #ifdef DEBUG
  Serial.print("UT = ");
  Serial.println(UT, 6);
  #endif

  // finally, calculate the local time of sunset in hours, LT
  LT = UT + TIMEZONE;

  // ensure LT is in the range 0-23.999999
  while (LT < 0) {
    LT = LT + 24.0;
  }
  while (LT >= 24.0) {
    LT = LT - 24.0;
  }

  #ifdef DEBUG
  Serial.print("LT = ");
  Serial.println(LT, 6);
  #endif

  // convert the local time of sunset in hours to time in seconds and return that value
  sunset = (long)(LT * 3600);
  return sunset;
}
