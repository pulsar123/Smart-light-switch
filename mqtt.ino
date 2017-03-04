void mqtt()
// Reading / sending MQTT message(s)
{
  if (MQTT_on)
  {
    // Receiving:
    client.loop();

    // Sending:
    if (mqtt_init == 1)
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
    if (light_state != light_state_old || mqtt_init == 1)
    {
      sprintf(buf, "%d", light_state);
      client.publish(ROOT"/light_state", buf);
#ifdef DEBUG
      Serial.print("light_state changed: ");
      Serial.println(light_state);
#endif
    }
    if (Mode != Mode_old || mqtt_init == 1)
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
    if (switch_state != switch_state_old || mqtt_init == 1)
    {
      sprintf(buf, "%d", switch_state);
      client.publish(ROOT"/switch_state", buf);
#ifdef DEBUG
      Serial.print("switch_state changed: ");
      Serial.println(switch_state);
#endif
    }
    mqtt_init = 0;
  }
}
