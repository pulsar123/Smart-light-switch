void get_time()
// Compute all time parameters
{

  // Arduino time in ms:
  t = millis();

  // Computing the on-hours based on internal timer:
  on_hours =  t / 3600000;
  // Clearing the abuse counters every on-hour:
  if (on_hours > on_hours_old)
  {
    if (switch_abuse == 0)
      switch_count = 0;
    if (mqtt_abuse == 0)
      mqtt_count = 0;
  }

  if (knows_time)
  {
    // Current (local winter time) day:
    Day = day();
    if (Day != Day_old)
    {
      // We recalculate time parameters every midnight (local winter time)
      redo_times = 1;
      // Once a day we reset timer (to avoid overflow)
      t_ntp = t;
    }
  }

  if (WiFi_on && (knows_time == 0 || redo_times == 1) && t >= t_ntp)
  {
    // Local winter time:
    local = ntpUnixTime(udp) + TIME_ZONE * 3600;
    long delta = 0;
    if (knows_time)
      // Deviation of the internal timer from the NTP time:
      delta = now() - local;
    // Ignore the new NTP time if the deviation is too large (>MAX_DELTA seconds):
    if (local > 0 && (knows_time == 0 || abs(delta) < MAX_DELTA))
    {
      // (Re)setting the internal timer to the NTP (local winter time) time:
      setTime(local);
      // A sanity check:
      if (year() < YEAR_MIN || year() > YEAR_MAX)
      {
#ifdef DEBUG
        Serial.print("Bad NTP year: ");
        Serial.print(year());
        Serial.print("; unxiTime=");
        Serial.println(local);
#endif
        if (knows_time == 0)
        {
          t_ntp = t + 60000; // Initial NTP failed, so trying again in 1 minute
          return;
        }
      }

      if (knows_time == 0)
      {
        // We connected to NTP for the first time since reboot, so switching to smart mode by default:
        Mode = 1;
        // If we at least once connect to NTP, time is considered to be known (even if later disconnected from WiFi):
        knows_time = 1;
      }
#ifdef DEBUG
      Serial.print("NTP time: ");
      Serial.println(local);
#endif
    }
    else
    {
      if (knows_time == 0)
      {
#ifdef DEBUG
        Serial.print("Bad NTP time: ");
        Serial.println(local);
#endif
        t_ntp = t + 60000; // Initial NTP failed, so trying again in 1 minute
        return;
      }
    }
  }

  if (knows_time == 1 && redo_times == 1)
    // We can only get to this point if the time is known. We get here initially, and then every midnight (local winter time).
    // Now we can compute all the time parameters which need to be re-computed daily
  {
    int Month = month();
    Day = day();
    // Number of minutes from the midnight to sunrise and sunset, for the current date (local winter time):
    dt_rise = mySunrise.Rise(Month, Day); // (month,day) - january=1
    dt_set = mySunrise.Set(Month, Day);
    // Current local winter time:
    local = now();
    // Local winter time corresponding to the last midnight:
    t_midnight = local - (3600 * hour() + 60 * minute() + second());
    // Sunrise and sunset time (with some random deviation):
    if (t_sunrise_next == 0)
      t_sunrise = t_midnight + 60 * dt_rise + deviation();
    else
      // If this is not the first calculation since reboot, we use the last day's calculation for the sunrise:
      t_sunrise = t_sunrise_next;
    t_sunset = t_midnight + 60 * dt_set + deviation();
    // We also need to know the next day's sunrise:
    unsigned long int t_next_day = local + 86400;
    int dt_rise2 = mySunrise.Rise(month(t_next_day), day(t_next_day));
    t_midnight2 = t_midnight + 86400;
    t_sunrise_next = t_midnight2 + 60 * dt_rise2 + deviation();
#ifdef DEBUG
    sprintf(tmp, "local=%d, mid=%d, sunrise=%d, sunset=%d, next=%d", local, t_midnight, t_sunrise, t_sunset, t_sunrise_next);
    Serial.println(tmp);
#endif

#ifdef INDOORS
    // Figuring out when to turn the indoor light off for the night, and on in the morning
    // +1 second to make T_1B moment inclusive:
    int delta = (int)(3600 * (T_1B - T_1A)) + 1;
    // Random moment to turn off the indoor light in the evening (from the T_1A...T_1B interval):
    t_1 = t_midnight + (int)(3600 * T_1A) + random(delta);
    // +1 second to make T_2B moment inclusive:
    delta = (int)(3600 * (T_2B - T_2A)) + 1;
    // Random moment to turn on the indoor light in the morning (from the T_2A...T_2B interval):
    if (t_2_next == 0)
      t_2 = t_midnight + (int)(3600 * T_2A) + random(delta);
    else
      // If this is not the first calculation since reboot, we use the last day's calculation for t_2:
      t_2 = t_2_next;
    t_2_next = t_midnight2 + (int)(3600 * T_2A) + random(delta);
#ifdef DEBUG
    sprintf(tmp, "t_2=%d, t_1=%d, t_2_next=%d", t_2, t_1, t_2_next);
    Serial.println(tmp);
#endif
#endif
    // Summer time correction (seconds); needs to be added to the local winter time to make proper (either winter or summer) local time:
    if (Zone.locIsDST(local))
      dt_summer = 3600;
    else
      dt_summer = 0;

    redo_times = 0;
  }

  return;
}



int deviation()
// Random deviation of the smart light on/off time from sunset or sunrise, in seconds
{
  return random(DARK_RAN) - (DARK_RAN - 1) / 2;
}

