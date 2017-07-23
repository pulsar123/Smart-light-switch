void mqtt()
// Reading / sending MQTT message(s)
{
  if (MQTT_on)
  {
    // Receiving:
    client.loop();

    // Sending:
    if (mqtt_init || mqtt_refresh)
    {
      if (bad_temp)
        client.publish(ROOT"/alarm", "1");
      else if (mqtt_abuse)
        client.publish(ROOT"/alarm", "2");
      else if (switch_abuse)
        client.publish(ROOT"/alarm", "3");
      else
        client.publish(ROOT"/alarm", "0");
    }
    if (light_state != light_state_old || mqtt_init || mqtt_refresh)
    {
      sprintf(buf, "%d", light_state);
      client.publish(ROOT"/light_state", buf);
#ifdef DEBUG
      Serial.print("light_state changed: ");
      Serial.println(light_state);
#endif
    }
    if (Mode != Mode_old || mqtt_init || mqtt_refresh)
    {
      sprintf(buf, "%d", Mode);
      client.publish(ROOT"/mode", buf);
      if (Mode == 1)
        // Just switched to Smart mode, so requesting an instant check inside smart() function
        instant_check = 1;
#ifdef DEBUG
      Serial.print("Mode changed: ");
      Serial.println(Mode);
#endif
    }
    if (switch_state != switch_state_old || mqtt_init || mqtt_refresh)
    {
      sprintf(buf, "%d", switch_state);
      client.publish(ROOT"/switch_state", buf);
#ifdef DEBUG
      Serial.print("switch_state changed: ");
      Serial.println(switch_state);
#endif
    }
    if ((mqtt_init || mqtt_refresh || t - t_mqtt > 60000) && knows_time == 1 && Mode == 1)
      // Sending time left every minute
    {
      t_mqtt = t;
      sprintf(buf, "%02d:%02d (in %02dh%02dm)", hrs_event, min_event, hrs_left, min_left);
      client.publish(ROOT"/left", buf);
    }
    mqtt_init = 0;
    mqtt_refresh = 0;
  }
}
