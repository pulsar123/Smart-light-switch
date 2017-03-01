void get_time()
// Get time from NTP regularly
{
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

  if (WiFi_on && t - t_ntp > DT_NTP)
  {
    unixTime = ntpUnixTime(udp) + TIME_ZONE * 3600;
    // Deviation of the internal timer from the NTP time:
    unsigned long delta = now() - unixTime;
    // Ignore the new NTP time if the deviation is too large (>MAX_DELTA seconds):
    if (unixTime > 0 && (knows_time == 0 || abs(delta) < MAX_DELTA))
    {
      // (Re)setting the internal timer to the NTP time:
      setTime(unixTime);
      // A sanity check:
      if (year() < YEAR_MIN || year() > YEAR_MAX)
      {
#ifdef DEBUG
        Serial.print("Bad NTP year: ");
        Serial.print(year());
        Serial.print("; unxiTime=");
        Serial.println(unixTime);
#endif
        t_ntp = t_ntp + 60000; // NTP failed, so trying again in 1 minute
        return;
      }
      if (knows_time == 0)
        // We connected to NTP for teh first time since reboot, so switching to smart mode by default:
        Mode = 1;
      // If we at least once connect to NTP, time is considered to be known (even if later disconnected from WiFi):
      knows_time = 1;
      t_ntp = t;
#ifdef DEBUG
      Serial.print("NTP time: ");
      Serial.println(unixTime);
#endif
    }
    else
    {
      t_ntp = t_ntp + 60000; // NTP failed, so trying again in 1 minute
    }
  }

  return;
}
