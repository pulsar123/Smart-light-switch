void read_switch()
// Reading the physical switch state, with de-bouncing, and triple-switch command detection.
// Updates switch_state, light_state, and Mode variables.
{
  byte switch_state_temp = 1 - digitalRead(SWITCH_PIN);

  if (switch_state_temp != switch_state && t - t_switch > DT_DEBOUNCE)
  {
    if (switch_state == 0 && switch_state_temp == 1)
      // Detecting the signal to flip the Mode (three off->on transitions within DT_MODE ms)
    {
      switch_count++; // Counting number of ON switches per hour, to detect abuse
      if (switch_count > N_ABUSE)
        // If switch abuse happened, raise the flag, switch to Smart mode, and ignore the physical switch until rebooted.
        // If time is not known, switch to Dumb mode and turn off the light
      {
        if (switch_abuse == 0)
        {
          switch_abuse = 1;
          Mode = knows_time;
          if (knows_time == 0)
            light_state = 0;
#ifdef DEBUG
          Serial.println("Switch abuse detected!");
#endif
        }
        return;
      }

      if (t - t_mode > DT_MODE)
        mode_count = 0;
      mode_count++;
#ifdef DEBUG
      Serial.print("mode_count=");
      Serial.println(mode_count);
#endif
      if (mode_count == 1)
        // Memorizing the first flip timing:
        t_mode = t;
      else if (mode_count == 3)
        // Three switch flips in a row, so reversing the Mode:
      {
        // Dumb->smart switch is only allowed if time is known:
        /* !!!!
                if (Mode == 1 || knows_time)
                {
                  Mode = 1 - Mode;
                  if (Mode == 1)
                    instant_check = 1;
          #ifdef DEBUG
                  Serial.print("New Mode=");
                  Serial.println(Mode);
          #endif
                }
        */
      }
    }
    switch_state = switch_state_temp;
    t_switch = t;
    if (Mode == 0)
      // Only in Dumb switch mode, light state reflects the physical switch state:
      light_state = switch_state;
  }
}


