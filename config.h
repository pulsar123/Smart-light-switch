/* Personal info (place the following 5 lines in a separate file, private.h, uncomment all the lines, and replace xxxx with your personal details):
  const char* ssid = "xxx";
  const char* password = "xxxx";
  const char* mqtt_server = "xxxx";
  const int TIME_ZONE = xx; // Time zone, hours; negative if west of Greenwich, positive if east; don't use summer/daylight saving time
  Sunrise mySunrise(xxx, xxx, TIME_ZONE);  // Your longitude and latutude; the latter is negative if west of Greenwich
*/

#include "private.h" //  Make sure you create this file first (see the lines above)

// The root name for MQTT topics:
#define ROOT "light1"
/*  MQTT topics:
  Incoming:
  ROOT"/switch1" : switching the mode (0: dumb; 1: smart);
  ROOT"/switch2" : manually switching the light (0: off; 1: on);

  Outgoing:
  ROOT"/mode"    : the mode (0: dumb; 1: smart);
  ROOT"/light_state"   : the light's state (on/off);
  ROOT"/switch_state"  : the physical switch's state (on/off)
  ROOT"/left"    : hours:minutes left before the next smart flip
  ROOT"/alarm"   : 1 if exceeded the critical temperature on SSR, 0 otherwise
  ROOT"/temp"    : current / historically maximum SSR temperature in C
*/


// Uncomment for the very first upload to the controller (this will only do one thing - initialize the EEPROM memory)
// Then comment it out again and upload the code one more time, for the actual functionality
//#define INITIALIZE
// Only for debugging (will use Serial interface to print messages):
//#define DEBUG
// Uncomment to use the two internal LEDs (WiFi status and warning signals)
#define USE_LEDS

const long DT_DEBOUNCE = 100; // Physical switch debounce time in ms
const long DT_MODE = 4000; // Number of ms for reading the Mode flipping signal (three off->on physical switch operations in a row)
const long DT_NTP = 86400000; // Time interval for NTP time re-syncing, ms
const long DT_DARK = 63000; // How often to do smart mode checking, in ms; better not be integer minutes, to add some randomness at seconds level
const int DARK_RAN = 11; // (DARK_RAN-1)/2 is the maximum deviation of the random smart light on/off from actual sunset/sunrise times, in minutes; should be odd for symmetry
const int DARK_SHIFT = 10; // Constant shift of the smart light on/off times into the darker part of the day (positive for sunset, negative for sunrise), in minutes
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

const float R_PULLDOWN = 45900; // Pulldown resistor (Ohms) used with the 50k thermistor on A0 pin
const float TH_A = 3.503602e-04; // Two coefficients for conversion of the thermistor resistance to temperature,
const float TH_B = 2.771397e-04; // 1/T = TH_A + TH_B* ln(R), where T is in Kelvins and R is in Ohms
// Analogue raw measurements when thermistor resistance is zero and infinity (should be 1023 and 0, but in reality different):
const int A0_ZERO = 995;
const int A0_INF = 1;

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
const int EEPROM_SIZE = 128; // Number of bytes allocated for EEPROM
// Addresses:
const int ADDR_TMAX = 0; // Tmax (maximum temperature ever recorded, and the date/time)
const int ADDR_BOOT = ADDR_TMAX + sizeof(Tmax_struc); // Booting counter

WiFiClient espClient;
PubSubClient client(espClient);
static WiFiUDP udp;
unsigned long unixTime;

long lastMsg = 0;
int value = 0;
byte led0, led1;
long t_led1;
byte light_state, light_state_old, Mode, switch_state, switch_state_old, Mode_old;
long t0, t, t_switch;
char buf[50], State_char;
byte WiFi_on, MQTT_on;
byte mode_count;
long t_mode, t_ntp, t_dark;
byte knows_time;
int dt_rise, dt_set, dt_now, dt_dev, dt_left, hrs_left, min_left, dt_event, hrs_event, min_event;
byte instant_check;
long t_a0;
float sum_T, T_avr;
int i_T, i_mqtt_T;
byte bad_temp;
int T_int, T_dec;
byte mqtt_init;
int Day, Day_old;
int switch_count, mqtt_count;
byte switch_abuse, mqtt_abuse;
long int on_hours, on_hours_old;
struct Tmax_struc Tmax;
