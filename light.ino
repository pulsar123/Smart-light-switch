void light()
// Operating the SSR (turning the lights on/off) based on light_state
{
  // SSR cannot be controlled if we went over the critical temperature:
  if (bad_temp)
    return;

  if (light_state != light_state_old && phys_flip == 0)
  {
    if (light_state == 0)
      // Turning off
    {
#ifdef SERVO
  Servo1.attach(SWITCH_PIN);
  Servo1.write(SERVO_OFF); 
  delay(SERVO_DELAY_MS);
  Servo1.detach();
#else
 #ifdef INVERT
      digitalWrite(SSR_PIN, HIGH);
 #else
      digitalWrite(SSR_PIN, LOW);
 #endif      
#endif 

#ifdef DEBUG
      Serial.println("SSR off");
#endif
    }
    else
      // Turning on
    {
#ifdef SERVO
  Servo1.attach(SWITCH_PIN);
  Servo1.write(SERVO_ON); 
  delay(SERVO_DELAY_MS);
  Servo1.detach();
#else
 #ifdef INVERT
      digitalWrite(SSR_PIN, LOW);
 #else
      digitalWrite(SSR_PIN, HIGH);
 #endif      
#endif

#ifdef DEBUG
      Serial.println("SSR on");
#endif
    }
  }

}


