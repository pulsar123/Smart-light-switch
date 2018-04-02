void led ()
// Operating the second internal LED, for warnings (if flashing, power cycling is required)
// Check the componenets first (solid state relay etc) - this could be a sign of the SSR
// overheating!
{
  if (bad_temp)
    // Overheating warning (short flashes)
  {
    if (t - t_led1 > DT_HEAT)
    {
      t_led1 = t;
      led1 = 1 - led1;
      digitalWrite(LED1, led1);
    }
  }

  else if (mqtt_abuse || switch_abuse)
    // Abuse warning (slow flashes)
  {
    if (t - t_led1 > DT_ABUSE)
    {
      t_led1 = t;
      led1 = 1 - led1;
      digitalWrite(LED1, led1);
    }
  }

  return;
}

