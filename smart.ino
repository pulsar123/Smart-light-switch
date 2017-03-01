void smart()
{
  // Smart functionality (turns on the light when it gets dark, and off when it gets bright, based on sunrise/sunset calculator)
  // Doing the checks regularly (every DT_DARK ms), or if a request of instant_check comes (from MQTT or physical switch)
  if (knows_time && (t - t_dark > DT_DARK || instant_check == 1))
  {
    instant_check = 0;

    Day = day();
    // We recompute the time deviation factor initially, and once per day (at midnight):
    if (Day != Day_old)
    {
      dt_dev = deviation();
#ifdef DEBUG
      Serial.print("dt_dev = ");
      Serial.println(dt_dev);
#endif
    }

    // Number of minutes from the midnight to sunrise and sunset, for the current date:
    dt_rise = mySunrise.Rise(month(), day()); // (month,day) - january=1
    dt_set = mySunrise.Set(month(), day()); // (month,day) - january=1
    // Number of minutes from the midnight to now:
    dt_now = 60 * hour() + minute();

    if (Mode == 1)
      // Smart mode
    {
      // Dark time criterion:
      if (light_state == 0 && (dt_now < dt_rise - dt_dev || dt_now > dt_set + dt_dev) && bad_temp == 0)
        light_state = 1;
      // Bright time criterion:
      else if (light_state == 1 && (dt_now >= dt_rise - dt_dev && dt_now <= dt_set + dt_dev))
        light_state = 0;
    }

    // How many minutes before the next smart flip:
    if (dt_now < dt_rise - dt_dev)
    {
      dt_event = dt_rise - dt_dev;
      dt_left = dt_event - dt_now;
    }
    else if (dt_now < dt_set + dt_dev)
    {
      dt_event = dt_set + dt_dev;
      dt_left = dt_event - dt_now;
    }
    else
    {
      dt_event = dt_rise - dt_dev;
      dt_left = 1440 - dt_now + dt_event;
    }
    hrs_left = dt_left / 60;
    min_left = dt_left % 60;
    hrs_event = dt_event / 60;
    min_event = dt_event % 60;
    if (MQTT_on)
    {
      sprintf(buf, "%02d:%02d (in %02dh%02dm)", hrs_event, min_event, hrs_left, min_left);
      client.publish(ROOT"/left", buf);
    }
#ifdef DEBUG
    Serial.print("Time: ");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print("; Day: ");
    Serial.print(day());
    Serial.print("; Month: ");
    Serial.print(month());
    Serial.print("; Year: ");
    Serial.println(year());
    Serial.print("Sunrise: ");
    Serial.print(dt_rise / 60);
    Serial.print(":");
    Serial.print(dt_rise % 60);
    Serial.print("; Sunset: ");
    Serial.print(dt_set / 60);
    Serial.print(":");
    Serial.print(dt_set % 60);
    Serial.print("; dt_dev=");
    Serial.println(dt_dev);
    Serial.print("Left: ");
    Serial.print(hrs_left);
    Serial.print("hrs, ");
    Serial.print(min_left);
    Serial.println(" min");
#endif
    t_dark = t;
  }
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int deviation()
// Deviation of the smart light on/off time from sunset or sunrise (constant + random parts), in minutes
{
  return DARK_SHIFT + random(DARK_RAN) - (DARK_RAN - 1) / 2;
}

