#pragma once

#include <lvgl.h>

void setup_screen(void);

void set_lamp_button_event_handler(lv_event_cb_t btn_event_handler);
/// @brief Returns the current lamp button state.
/// @returns true for checked, false for un-checked
bool get_lamp_button_state();

/// @brief Set the lamp button state
/// @param state: true for on, false for off
void set_lamp_button_state(bool state);

void update_time_label();

void update_outside_temp(const float temp, const float humid, const float baro);

void update_fam_room_temp(const float temp);

void update_temp_humid_display(float temp_fahren, float temp_set);

class DisplayElemIfc
{
public:
  virtual ~DisplayElemIfc() {};

  /// @brief Update the value of the field.
  /// @param val
  virtual void update(float val) = 0;

}; // class DisplayElemIfc

class SetTempDE : public DisplayElemIfc
{
public:
  SetTempDE() {};
  ~SetTempDE() override {};

  virtual void update(float set_temp) override;

private:
  int _prev_tset {0};

}; // class SetTempDE

extern SetTempDE lr_set_temp_DE;
