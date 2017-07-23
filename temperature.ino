void temperature()
// Measuring the SSR's temperature, and disabling it if it's too hot
{
  if (t - t_a0 > DT_TH)
  {
    float Raw = (float)analogRead(TH_PIN);
    float R;
#ifdef TH_PULLUP
    // Thermistor resistance (assuming it is used with a pullup resistor), in Ohms:
    R = R_PULL * (Raw - A0_LOW) / (A0_HIGH - Raw);
    // Correcting for the weird board artifact - it appears to have an internal pulldown resistor on A0 pin, with R_INETRNAL value:
    R = 1.0 / (1.0 / R - 1.0 / R_INTERNAL);
#else
    // Thermistor resistance (assuming it is used with a pulldown resistor), in Ohms:
    // Correcting for the weird board artifact - it appears to have an internal pulldown resistor on A0 pin, with R_INETRNAL value:
    R = 1.0 / (1.0 / R_INTERNAL + 1.0 / R_PULL) * ((A0_HIGH - 1.0) / (Raw - A0_LOW) - 1.0);
#endif
    // Temperature (Celsius):
    float T = 1.0 / (TH_A + TH_B * log(R)) - 273.15;
    sum_T = sum_T + T;
    i_T++;
    t_a0 = t;
    if (i_T == N_T)
    {
      T_avr = sum_T / N_T;
      int T1 = (int)(T_avr * 10.0 + 0.5);
      T_int = T1 / 10;
      T_dec = T1 % 10;
      int R_int = (int)R;
      i_T = 0;
      sum_T = 0.0;
      if (bad_temp == 0 && T_avr > T_MAX)
        // We exceeded the critical SSR temperature; disabling SSR until it's rebooted:
      {
        bad_temp = 1;
        light_state = 0;
        digitalWrite(SSR_PIN, LOW);
#ifdef DEBUG
        Serial.println("T>T_MAX! Disabling SSR");
#endif
        if (MQTT_on)
          client.publish(ROOT"/alarm", "1");
      }

      // Storing the highest ever temperature to EEPROM
      if (T_avr > Tmax.T)
      {
        Tmax.T = T_avr;
        Tmax.Day = day();
        Tmax.Month = month();
        Tmax.Year = year();
        Tmax.Hour = hour();
        Tmax.Min = minute();
        EEPROM.put(ADDR_TMAX, Tmax);
        EEPROM.commit();
      }

      if (MQTT_on)
      {
        i_mqtt_T++;
        // Sending current temperature every 30 seconds:
        if (i_mqtt_T == 30)
        {
          i_mqtt_T = 0;
          int T_max1 = (int)(Tmax.T * 10.0 + 0.5);
          int Tmax_int = T_max1 / 10;
          int Tmax_dec = T_max1 % 10;
          sprintf(buf, "%d.%01d/%d.%01d", T_int, T_dec, Tmax_int, Tmax_dec);
          client.publish(ROOT"/temp", buf);
        }
      }

#ifdef DEBUG
      // Uncomment to measure the A0_LOW and A0_HIGH parameters (raw A0 pin values when it is pulled down and up, respectively)
/*
      Serial.print("+++ A0: ");
      Serial.print(analogRead(TH_PIN));
      Serial.print(", R=");
      Serial.print(R_int);
      Serial.print("; T=");
      Serial.print(T_int);
      Serial.print(".");
      Serial.print(T_dec);
      Serial.println(" C");
*/
#endif
    }
  }
}
