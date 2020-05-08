# LandscapeLightingTimerDimmer

This code is for an Arduino-based timer/dimmer for low-voltage landscape lights, specifically, those that can run on DC current instead of the usual 12 VAC.

I’m running it at my house using an Arduino Duemilanove board with an Ethernet shield (required for setting the time) and a datalogger shield that has a DS1307 real-time clock chip on it, which I use to minimize my load on the network time servers while keeping the crummy Arduino clock more or less on time.

I’m using a 12 VDC, 20A power supply as the high-current source for the landscape lights:
https://www.amazon.com/gp/product/B01E6S0JS4/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1

And a field-effect transistor (FET) switch to provide pulsed DC power to the lights:
https://www.amazon.com/gp/product/B085VD5YZJ/ref=ppx_yo_dt_b_asin_title_o00_s00?ie=UTF8&psc=1

The software sets the DS1307 clock chip on startup and once per day thereafter using time info off the Internet, specifically, from NIST’s NTP servers.  The DS1307 is used to keep the Arduino clock on time by resyncing every 10 minutes.  (NIST is the U.S. National Institute of Standards and Technology, part of the U.S. Department of Commerce.  But no praise for Wilbur Ross is intended or implied.)

The lights are turned on slowly at sunset + a delay, and turned off slowly at 9:30PM.  What I mean by “slowly” is over a period of five minutes.

I originally used a table of sunset times derived from a NOAA spreadsheet to determine when sunset occurs each day, but then I found a U.S. Naval Observatory algorithm for computing sunset times, so now I’m using it.  But the software can be compiled to use either the table or the USNO algorithm.  The algorithm appears in the United States Naval Observatory's Almanac for Computers, 1990.

An Arduino “analog” (actually pulse-width modulated digital) signal from pin 9 is used to drive the FET switch.
