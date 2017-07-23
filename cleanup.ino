void cleanup()
{
  switch_state_old = switch_state;
  light_state_old = light_state;
  Mode_old = Mode;
  on_hours_old = on_hours;
  if (knows_time)
    Day_old = Day;
  return;
}
