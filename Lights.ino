/*
  WARNING: don't use an external +5V PSU (connected to GND and +5V pins of ESP board) and at the same time connect the board to a computer
  via USB - this can damage the controller and potentially the computer. Use either one or the other one, not both together.

  The code is designed for cheap (3-4$ from ebay) ESP8266 based boards NodeMCU devkit v0.9 with a micro-USB connector (based on CH340G
  chip; one needs to install the CH340 driver to your computer). E.g.: http://www.ebay.ca/itm/262136737901 .

  The detailed description: http://pulsar124.wikia.com/wiki/Smart_light_switch

  ESP8266 code for a smart light switch. The ESP controller operates the light via a solid state relay,
  and reads the state of the physical light switch. It can be either in Smart mode (turns on during the dark
  time, using an internal sunrise/set calculator and NTP date/time from Internet), or Dumb mode (manual operation
  with the physical switch or from MQTT client). One can switch between the two modes by turning on/off the physical switch 3 times
  over <4 seconds interval. It uses MQTT protocol to communicate with the home automation system. (E.g.
  mosquitto MQTT server + OpenHab web server.)

  WiFi/MQTT/NTP connections are non-blocking (asynchronous). After rebooting, the switch is in the "dumb" mode, until
  WiFi and NTP connections are made. One cannot switch it to "smart" mode (from the physical switch or MQTT) during this
  time. Once the current time is obtained from NTP, the switch automatically switches to the "smart" mode.

  If at any time WiFi and/or MQTT connections are lost, switch will continue operate using its internal timer (in either dumb or smart mode). One
  can flip smart <-> dumb mode using the physical switch. It will keep trying to reconnect to WiFi and/or MQTT until it succeeds.

  It will regularly (evey 24h) reconnect to NTP time server (if WiFi is connected), to maintain the accuracy of its clock. If it can't,
  it will operate normally using its internal clock, until the NTP connection succeeds.

  The only analogue read pin of ESP board (A0) is connected to a voltage divider formed by a 50k thermistor (NTC MF58 3950 B 50K from ebay; 10 pcs
  for 1$) and a 47k regular resistor (the actual resistance in Ohms should be enetered in config.h as R_PULL), with the thermistor connected to
  +3.3V. The thermistor should be glued to the solid state relay. It will report the temperature of the SSR to the controller (and to the MQTT network).
  If the temperature goes over T_MAX=50C (config.h), the SSR will be turned off until the controller is power cycled. (LED1 will be flashing
  (fast) in case of overheating.) This is a safety feature
  to prevent overheating and potential fire from SSR. You can use a different thermistor, but you will have to calibrate it (obtain the values
  for TH_A and TH_B in config.h) yourself.

  It is also higly recommended to connect a thermal fuse (say, for 130C) right before the SSR, and glue it to the SSR. This is the last resort protection
  against SSR overheating if everything esle fails (controller and/or thermistor and/or PSU).

  One has to use an external pullup resistor of 3.3k for the SWITCH_PIN (D1) - just solder the resistor between D1 and +3.3V. The internal pullup resistor is very weak
  (~100k), and does not suppress noise when the switch is placed next to AC wiring, resulting in frequent (every second or more) false switch on/off readings. 3.3k value
  should be enough to completely remove false switching.

  When you install the code for the first time, uncomment the "#define INITIALIZE" line in config.h - this will initialize the flash (EEPROM) memory on the controller.
  After that comment the line out and re-upload the code, for normal operation.

  EEPROM memory is used to store some data, e.g. the hystorically highest temperature recorded, and the date/time when it happened (structure Tmax), also the total number of
  reboots (N_boot). The hystorically largest temperature is constantly communicated to MQTT, along with the current temperature. To see the other EEPROM data, and also lots of
  debugging information, recompile the code with the "#DEBUG" line uncommented in config.h,  connect the ESP to a computer via USB (but see the WARNING at the top!),
  and launch Serial Monitor from Arduino IDE.

  The code has abuse protection features - both for the physical switch, and for MQTT switches. If any of these switches is used more than 100 times in any given hour,
  the corresponding functionality is disabled, and the code switches to smart mode (if time is known), or dumb mode with the light turned off (if time is not known).
  Also, LED1 starts flashing (slowly). To recover from this mode one needs to power cycle the controller.

  You will have to adjust many parameters in config.h to your specific setup.

  To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

  Libraries used:
  https://github.com/knolleary/pubsubclient
  https://github.com/ekstrand/ESP8266wifi
  https://github.com/PaulStoffregen/Time

  I already included (customised) files from the following library, so you shouldn't install it separately:
  https://github.com/chaeplin/Sunrise

  SSR: 250V / 2A (1$ on ebay: http://www.ebay.ca/itm/282227077259 ; the 5V model). Turn-on voltage is as low as 2.0V (so perfectly
    fine to be used with 3.3V logic, like in ESP8266).
     - 60W/120V (0.5A) load, warms up by +17C (room temp +21C); the voltage drop is 1.25V (so 0.6W dissipated).
     - 40W/120V (0.33A) load; warm up by +9C; voltage drop 1.05V (0.35W dissipation).

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>
#include <EEPROM.h>
#include "Sunrise2.h"
#include "config.h"


//++++++++++++++++++++++++++ SETUP +++++++++++++++++++++++++++++++++++

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  delay(100);
#endif
  pinMode(SSR_PIN, OUTPUT);
#ifdef PHYS_SWITCH
  pinMode(SWITCH_PIN, INPUT_PULLUP);
#endif  
  knows_time = 0;
  WiFi_on = 0;
  MQTT_on = 0;
  // Initially the switch is in Dumb mode:
  Mode = 0;
  Mode_old = Mode;
  mode_count = 0;
  instant_check = 0;
#ifdef PHYS_SWITCH
  switch_state = 1 - digitalRead(SWITCH_PIN); // reading the current state of the physical switch
  switch_state_old = switch_state;
  light_state = switch_state; // Initially the light state = physical switch state (dumb mode)
#else
  light_state = 0;
#endif    
  light_state_old = light_state;
//  pinMode(TH_PIN, INPUT);
#ifdef WIFI_LED
  pinMode(LED0, OUTPUT);     // Initialize the BUILTIN_LED pin as an output (WiFi connection indicator)
#endif
  pinMode(LED1, OUTPUT);  // warning indicator (will flash if power cycling is needed - if overheated or abused)
  delay(10);
  led0 = HIGH;
  led1 = HIGH;
  digitalWrite(SSR_PIN, switch_state); // Initially the SSR state = physical switch state (dumb mode)
#ifdef WIFI_LED
  digitalWrite(LED0, led0);
#endif
  digitalWrite(LED1, led1);
  WiFi.begin(ssid, password);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  t0 = millis();
  t = t0;
  t_switch = t0 - DT_DEBOUNCE - 1;
  t_mode = t0 - DT_MODE - 1;
  t_ntp = t0 - DT_NTP - 1;
  t_dark = t0 - DT_DARK - 1;
  t_a0 = t0;
  t_led1 = t0;
  sum_T = 0.0;
  i_T = 0;
  bad_temp = 0;
  T_avr = 0;
  mqtt_init = 1;
  i_mqtt_T = 0;
  dt_dev = deviation();
  mySunrise.Custom(Z_ANGLE);
  Day_old = -2;
  dt_dev = 0;
  switch_count = 0;
  switch_abuse = 0;
  mqtt_count = 0;
  mqtt_abuse = 0;
  on_hours = 0;
  on_hours_old = 0;
  phys_flip = 0;
  mqtt_refresh = 0;

  // EEPROM stuff:
  EEPROM.begin(EEPROM_SIZE);
#ifdef INITIALIZE
#ifdef DEBUG
  Serial.println("Initializing...");
#endif
  Tmax = { -100, 0, 0, 0, 0, 0};
  EEPROM.put(ADDR_TMAX, Tmax);
  EEPROM.put(ADDR_BOOT, (int)0);
  EEPROM.commit();
#else
  EEPROM.get(ADDR_TMAX, Tmax);
  // Increamenting the booting counter:
  int N_boot;
  EEPROM.get(ADDR_BOOT, N_boot);
  N_boot++;
  EEPROM.put(ADDR_BOOT, N_boot);
  EEPROM.commit();
#ifdef DEBUG
  int T1 = (int)(Tmax.T * 10.0 + 0.5);
  T_int = T1 / 10;
  T_dec = T1 % 10;
  sprintf(buf, "Tmax=%d.%01d, on %02d/%02d/%d, at %02d:%02d", T_int, T_dec, Tmax.Day, Tmax.Month, Tmax.Year, Tmax.Hour, Tmax.Min);
  Serial.println(buf);
  Serial.print("N_boot=");
  Serial.println(N_boot);
  Serial.println("Setup is done");
#endif
#endif
}


//+++++++++++++++++++++++++ MAIN LOOP +++++++++++++++++++++++++++++++

void loop()
{
#ifdef INITIALIZE
  return;
#endif

  // Establishing and re-establishing WiFi and MQTT connections:
  connections();

  t = millis();

  // Get time from NTP
  get_time();

  // Reading the physical switch state (with debouncing):
  read_switch();

  // Smart functionality:
  smart();

  // Measuring the SSR's temperature, and disabling it if it's too hot:
  temperature();

  // Reading / sending MQTT message(s):
  mqtt();

  // Switching the light on or off when needed:
  light();

  led();

  switch_state_old = switch_state;
  light_state_old = light_state;
  Mode_old = Mode;
  on_hours_old = on_hours;
  if (knows_time)
    Day_old = Day;
}

