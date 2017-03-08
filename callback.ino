void callback(char* topic, byte* payload, unsigned int length)
// The function processing incoming MQTT commands.
// The command is in payload[0..length-1] char array.
{

  if (mqtt_count > N_ABUSE)
    // If mqtt switch abuse happened, raise the flag, switch to Smart mode, and ignore the mqtt switches until rebooted
    // If time is not known, switch to Dumb mode and turn off the light
  {
    if (mqtt_abuse == 0)
    {
      mqtt_abuse = 1;
      Mode = knows_time;
      if (knows_time == 0)
        light_state = 0;
      if (MQTT_on)
        client.publish(ROOT"/alarm", "2");
#ifdef DEBUG
      Serial.println("MQTT abuse detected!");
#endif
    }
    return;
  }

  // Dumb (0) / Smart (1) mode switch
  if (strcmp(topic, ROOT"/switch1") == 0)
  {
    mqtt_count++; // Counting number of mqtt switches per hour, to detect abuse

    if (Mode == 1 && (char)payload[0] == '0')
      // Dumb switch
    {
      Mode = 0;;
#ifdef DEBUG
      Serial.print("New Mode=");
      Serial.println(Mode);
#endif
    }
    else if (knows_time && Mode == 0 && (char)payload[0] == '1')
      // Smart switch (only allowed if time is known)
    {
      Mode = 1;
#ifdef DEBUG
      Serial.print("New Mode=");
      Serial.println(Mode);
#endif
    }
  }

  // Manual light switch (only in dumb mode)
  if (Mode == 0 && strcmp(topic, ROOT"/switch2") == 0)
  {
    mqtt_count++; // Counting number of mqtt switches per hour, to detect abuse

    if (light_state == 1 && (char)payload[0] == '0')
      // Light off
    {
      light_state = 0;;
#ifdef DEBUG
      Serial.print("New light_state=");
      Serial.println(light_state);
#endif
    }
    else if (light_state == 0 && (char)payload[0] == '1' && bad_temp == 0)
      // Light on
    {
      light_state = 1;
#ifdef DEBUG
      Serial.print("New light_state=");
      Serial.println(light_state);
#endif
    }
  }

  if (strcmp(topic, "openhab/start") == 0 && (char)payload[0] == '1')
    // We received a signal that openhab server has just restarted, so will re-publish all mqtt states later, in mqtt()
  {
    mqtt_refresh = 1;
  }

  return;
}

