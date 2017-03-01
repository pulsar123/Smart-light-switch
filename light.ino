void light()
// Operating the SSR (turningg the lights on/off) based on light_state
{
// SSR cannot be controlled if we went over the critical temperature:  
  if (bad_temp)
    return;
  
  if (light_state != light_state_old)
    if (light_state == 0)
      // Turning off
    {
      digitalWrite(SSR_PIN, LOW);
#ifdef USE_LEDS
#ifdef DEBUG
      digitalWrite(LED1, HIGH);
#endif      
#endif      
#ifdef DEBUG
      Serial.println("SSR off");
#endif
    }
    else
      // Turning on
    {
      digitalWrite(SSR_PIN, HIGH);
#ifdef USE_LEDS  
#ifdef DEBUG
      digitalWrite(LED1, LOW);
#endif
#endif      
#ifdef DEBUG
      Serial.println("SSR on");
#endif
    }

}


