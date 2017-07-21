/* The switch number; use this to keep separate profiles for your switches
   1: Front light
   2: Back light
   3: 1st floor
   4: 2nd floor
   ...
*/
#define N_SWITCH 3

/* Personal info (place the following 5 lines in a separate file, private.h, uncomment all the lines, and replace xxxx with your personal details):
  const char* ssid = "xxx";
  const char* password = "xxxx";
  const char* mqtt_server = "xxxx";
  const int TIME_ZONE = xx; // Time zone, hours; negative if west of Greenwich, positive if east; don't use summer/daylight saving time
  Sunrise2 mySunrise(xxx, xxx, TIME_ZONE);  // Your longitude and latutude; the latter is negative if west of Greenwich
*/
#include "private.h" //  Make sure you create this file first (see the lines above)


#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

// The root name for MQTT topics; contains N_SWITCH parameter as the last character:
#define ROOT "light" STRINGIZE(N_SWITCH)
/*  MQTT topics:
  Incoming:
  ROOT"/switch1" : switching the mode (0: dumb; 1: smart);
  ROOT"/switch2" : manually switching the light (0: off; 1: on);
  openhab/start  : optional; if "1" is received (the sign that openhab has just re-started), the switch will re-publish its current state

  Outgoing:
  ROOT"/mode"    : the mode (0: dumb; 1: smart);
  ROOT"/light_state"   : the light's state (on/off);
  ROOT"/switch_state"  : the physical switch's state (on/off)
  ROOT"/left"    : hours:minutes left before the next smart flip
  ROOT"/alarm"   : 1 if exceeded the critical temperature on SSR, 2 for MQTT abuse, 3 for phys. switch abuse, 0 otherwise
  ROOT"/temp"    : current / historically maximum SSR temperature in C

  For the "openhab/start" thing to work, one needs to have the OpenHab to publish "1" (followed by "0") in this topic at startup.
  Under Windows this can be accomplished by adding this line before the last line of openhab.bat file:

  start /b C:\openHAB2\mqtt_start.bat >nul

  and creating a new file mqtt_start.bat with the following content:

  timeout /t 20 /nobreak
  C:\mosquitto\mosquitto_pub.exe -h 127.0.0.1 -t "openhab/start" -m "1"
  C:\mosquitto\mosquitto_pub.exe -h 127.0.0.1 -t "openhab/start" -m "0"
*/


// Uncomment for the very first upload to the controller (this will only do one thing - initialize the EEPROM memory)
// Then comment it out again and upload the code one more time, for the actual functionality
//#define INITIALIZE
// Only for debugging (will use Serial interface to print messages):
//#define DEBUG
// Uncomment to use one internal LEDs as WiFi status (the other internal LED will only be used for warning signals)
//#define WIFI_LED

const long DT_DEBOUNCE = 100; // Physical switch debounce time in ms
const long DT_MODE = 4000; // Number of ms for reading the Mode flipping signal (three off->on physical switch operations in a row)
const long DT_NTP = 86400000; // Time interval for NTP time re-syncing, ms
const int DARK_RAN = 601; // (DARK_RAN-1)/2 is the maximum deviation of the random smart light on/off from actual sunset/sunrise times, in seconds; should be odd for symmetry
const long DT_TH = 100; // raw temperarture measurement interval, ms
const int N_T = 10; // average temperature over this many measurements (so the actual temperature is updated every N_T*DT_TH ms)
const float T_MAX = 50.0; // Maximum allowed SSR temperature (C); if larger, the SSR will be disabled until reboot time, and LED1 will start slowly flashing
// Maximum number of physical off-on switching in any given hour; if exceeded, the program goes into a safe mode (smart switch, physical switch ignored - if the time is known;
// dumb mode, light off - if the time is not known). This is protection against switch reading noise.
// Also used to protect from hacking (MQTT switching limit per hour)
const int N_ABUSE = 100;
const long DT_HEAT = 250; // Number of milliseconds for overheating warning flashings (LED1), for on and off states
const long DT_ABUSE = 1000; // Number of milliseconds for abuse (mqtt or switch) warning flashings (LED1), for on and off states

// Good years (used to verify if NTP output is sensible). Switch will not work properly if the current year is outside of this range
const int YEAR_MIN = 2017;
const int YEAR_MAX = 2037;
const unsigned long MAX_DELTA = 600; // If the new NTP time deviates from the internal timer by more than this value (in seconds), ignore the new NTP time


// Custom profiles for different switches:
//----------------------------------------- Switch 1 (Front light) ------------------------------------------
#if N_SWITCH == 1
// Uncomment if you want to use a physical switch:
#define PHYS_SWITCH
// Uncomment if the lights are indoors. This will turn off the lights during the night. The lights will be on in the evening (after sunset) and morning (before sunrise).
//#define INDOORS
// The light will be switched off for the night at a random time between T_1A and T_1B (in hours; 24-hours clock; local time):
// (Only matters if INDOORS is defined above)
const float T_1A = 0;
const float T_1B = 0;
// The light will be switched on in the morning at a random time between T_2A and T_2B (in hours; 24-hours clock):
// (Only matters if INDOORS is defined above)
const float T_2A = 0;
const float T_2B = 0;
// Custom z-angle (in degrees) for the sunset/sunrise calculations. This is the angle of the Sun's center below the horizon (in the absense of refraction).
// Z=0.5,6,12,18 correspond to Actual, Civil, Nautical, and Astronomical sunset from Sunset.h library. It can also be negative (Sun is above the horizon).
// The larger the number is, the darker it gets.
const float Z_ANGLE = 4;
/* Thermistor can be connected to A0 pin with either a pulldown or pullup resistor, 47k in both cases.
   My original design used a pulldown resistor, but pullup is more economical as on can share the common ground
   between the thermistor and solid state relay control - meaning only 3 (vs 4) wires from ESP to the SSR/thermistor bundle.
   In terms of the temperature accuracy it shouldn't make a difference.
   TH_PULLUP should be defined for the pullup scenario; comment it our for the pulldown scenario
*/
//#define TH_PULLUP
const float R_PULL = 45900; // The actual (measured) resistance of the pulldown or pullup  resistor (Ohms) used with the 50k thermistor on A0 pin. Use ~47k for the best temperature accuracy.
/* NodeMCU devkit v0.9 uses an internal voltage divider based on two resistors - 100k and 220k - to convert the 0...3.3V input voltage range
   to 0...1V range required by the ESP chip (https://github.com/nodemcu/nodemcu-devkit). This creats a pulldown resistor of 320k on A0 pin.
   This needs to be corrected for. If your board doesn't have this issue, put a very large number here (1e9). But then it is your responsibility
   to wire your thermistor in such a way that ESP chip will not get >1V.
*/
const float R_INTERNAL = 320000;
const float TH_A = 3.503602e-04; // Two coefficients for conversion of the thermistor resistance to temperature,
const float TH_B = 2.771397e-04; // 1/T = TH_A + TH_B* ln(R), where T is in Kelvins and R is in Ohms
// Analogue raw measurements on pin A0, corresponding to high (connected to +3.3V directly - important because of the internal voltage divider on A0)
// and low (connected to ground directly) situations
// In theory should be 1023 and 0, respectively, but for some reason the real values are slightly different
const int A0_HIGH = 995;
const int A0_LOW = 1;

//----------------------------------------- Switch 2 (Back light) ------------------------------------------
#elif N_SWITCH == 2
//ADC_MODE(ADC_TOUT_3V3);
#define PHYS_SWITCH
//#define INDOORS
const float T_1A = 0;
const float T_1B = 0;
const float T_2A = 0;
const float T_2B = 0;
const float Z_ANGLE = 4;
#define TH_PULLUP
const float R_PULL = 45800;
const float R_INTERNAL = 320000;
const float TH_A = 3.503602e-04;
const float TH_B = 2.771397e-04;
const int A0_HIGH = 996;
const int A0_LOW = 0;

//----------------------------------------- Switch 3 (1st floor) ------------------------------------------
#elif N_SWITCH == 3
//#define PHYS_SWITCH
#define INDOORS
// The light will be switched off for the night at a random time between T_1A and T_1B (in hours; 24-hours clock):
// (Only matters if INDOORS is defined above)
const float T_1A = 21.5;
const float T_1B = 22.0;
// The light will be switched on in the morning at a random time between T_2A and T_2B (in hours; 24-hours clock):
// (Only matters if INDOORS is defined above)
const float T_2A = 5.5;
const float T_2B = 6.0;
const float Z_ANGLE = 1;
#define TH_PULLUP
const float R_PULL = 46000;
const float R_INTERNAL = 320000;
const float TH_A = 3.503602e-04;
const float TH_B = 2.771397e-04;
const int A0_HIGH = 996;
const int A0_LOW = 0;
#endif
//----------------------------------------------------------------------------------------------------------


// Pins used (NodeMCU devkit v0.9; e.g. http://www.ebay.ca/itm/272526162060)
// (See http://cdn.frightanic.com/blog/wp-content/uploads/2015/09/esp8266-nodemcu-dev-kit-v1-pins.png for NodeMCU devkit v0.9 pinout)
// Pin used to read the state of the physical switch (make sure to use an external 3.3k pullup resistor, to suppress reading noise):
const byte SWITCH_PIN = 5;  // D1
// Pin to operate solid state relay (SSR):
const byte SSR_PIN = 4; // D2
// Analogue pin for thermistor:
const byte TH_PIN = A0;
// Internal pins (do not connect anything there):
// Internal LED pin is used to indicate when connected to WiFi:
const byte LED0 = BUILTIN_LED;  // D0
// Internal LED for warning signal (overheating: fast flashing; abuse: slow flashing)
const byte LED1 = 2; // D4



//+++++++++++++++++++++++++++++ Normally nothing should be changed below ++++++++++++++++++++++++++++++++++++++++++++++++++


// Structure used to store the historically maximum temperature and the corresponding date/time in EEPROM:
struct Tmax_struc
{
  float T;
  int Day;
  int Month;
  int Year;
  int Hour;
  int Min;
};

// EEPROM stuff
const int EEPROM_SIZE = 256; // Number of bytes allocated for EEPROM
// Addresses:
const int ADDR_TMAX = 0; // Tmax (maximum temperature ever recorded, and the date/time)
const int ADDR_BOOT = ADDR_TMAX + sizeof(Tmax_struc); // Booting counter

WiFiClient espClient;
PubSubClient client(espClient);
static WiFiUDP udp;
unsigned long locarel;

long lastMsg = 0;
int value = 0;
byte led0, led1;
unsigned long t_led1;
byte light_state, light_state_old, Mode, switch_state, switch_state_old, Mode_old;
unsigned long t0, t, t_switch;
char buf[50], State_char;
byte WiFi_on, MQTT_on;
byte mode_count;
unsigned long t_mode, t_ntp, t_sunrise, t_sunset, t_sunrise2, dt_summer;
byte knows_time, redo_times;
int dt_rise, dt_set, dt_now, dt_dev, dt_left, hrs_left, min_left, dt_event, hrs_event, min_event;
int local;
byte instant_check;
unsigned long t_a0;
float sum_T, T_avr;
int i_T, i_mqtt_T;
byte bad_temp;
int T_int, T_dec;
byte mqtt_init;
int Hour, Hour_old;
int switch_count, mqtt_count;
byte switch_abuse, mqtt_abuse;
unsigned long int on_hours, on_hours_old;
struct Tmax_struc Tmax;
byte phys_flip;
byte mqtt_refresh;
#ifdef INDOORS
long int t_1, t_2;
#endif

  TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -240};  //UTC - 4 hours
  TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -300};   //UTC - 5 hours  
  Timezone usEastern(usEDT, usEST);

