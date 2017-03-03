void connections()
// Establishing and re-establishing WiFi and MQTT connections
{
  // Taking care of WiFi connection (trying to reconnect when lost)
  if (WiFi_on == 0 && WiFi.status() == WL_CONNECTED)
  {
    WiFi_on = 1;
    // Turning the LED0 on when WiFi is connected:
#ifdef WIFI_LED
    digitalWrite(LED0, LOW);
#endif    
#ifdef DEBUG
    Serial.println("WiFi on");
#endif
  }

  if (WiFi_on == 1 && WiFi.status() != WL_CONNECTED)
    // If we just lost WiFi (will have to reconnect MQTT as well):
  {
    WiFi_on = 0;
    MQTT_on = 0;
    mqtt_init = 1;
    WiFi.begin(ssid, password);
#ifdef WIFI_LED
    digitalWrite(LED0, HIGH);
#endif    
#ifdef DEBUG
    Serial.println("WiFi off");
#endif
  }

  if (WiFi_on == 1 && !client.connected())
    // If we just lost MQTT connection (WiFi is still on), or not connected yet after WiFi reconnection:
  {
    MQTT_on = 0;
    mqtt_init = 1;
#ifdef DEBUG
    Serial.println("MQTT off");
#endif
  }

  // Taking care of MQTT connection (reconnecting when lost and only if WiFi is connected)
  if (WiFi_on == 1 && MQTT_on == 0)
  {
    if (client.connect(ROOT))
    {
      client.subscribe(ROOT"/switch1");
      client.subscribe(ROOT"/switch2");
      MQTT_on = 1;
#ifdef DEBUG
      Serial.println("MQTT on");
#endif
    }
  }
}
