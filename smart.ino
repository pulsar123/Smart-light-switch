void smart()
{

  // Local winter time:
  unsigned long int local = now();

  if (knows_time == 1)
  {
    // Dark time criterion:
    if (Mode == 1)
      if (local < t_sunrise || local > t_sunset)
        light_state = 1;
      else
        light_state = 0;

    unsigned long int left, event;
    // Finding the next event:
    if (local < t_sunrise)
      event = t_sunrise;
    else if (local < t_sunset)
      event = t_sunset;
    else
      event = t_sunrise_next;
    // How many second left till the next event:
    left = event - local;
#ifdef INDOORS
    // Turning an interior light off for the night:
    if (Mode == 1)
      if (local > t_1 || local < t_2)
        light_state = 0;
    if (local < t_2 && t_2 - local < left)
      event = t_2;
    else if (local < t_1 && t_1 - local < left)
      event = t_1;
    else if (t_2_next - local < left)
      event = t_2_next;
    left = event - local;
#endif
    // How many hours and minute left till the next state change:
    hrs_left = left / 3600;
    min_left = (left % 3600) / 60;
    // Local time for the next state change:
    if (event + dt_summer < t_midnight2)
      // The event is today
    {
      hrs_event = (event + dt_summer - t_midnight) / 3600;
      min_event = ((event + dt_summer - t_midnight) % 3600) / 60;
    }
    else
      // The event is next day
    {
      hrs_event = (event + dt_summer - t_midnight2) / 3600;
      min_event = ((event + dt_summer - t_midnight2) % 3600) / 60;
    }
  }

  if (bad_temp)
    // If SSR is overheated, the light is always off:
    light_state = 0;

  return;
}

