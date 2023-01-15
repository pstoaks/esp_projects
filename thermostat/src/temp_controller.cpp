#include "temp_controller.h"

#include "http_request.h"
#include "screen1.h"
#include <ESP32Encoder.h>

void TempController::init()
{
  // Update the encoder position based on the new set temp.
  _encoder.setCount(_temp_to_count(_set_temp));
} // init()

bool TempController::update()
{
  // Early return if it isn't time to do something
  if ( !_upd_timer.expired() )
    return false;

  // Check to see if the encoder position has changed.
  static unsigned long oldPosition  = _encoder.getCount();
  unsigned long newPosition = oldPosition;
  newPosition = _encoder.getCount();
  if ( newPosition != oldPosition )
  {
    // Detected knob movement
    _position_changing = true;
    _position_changing_tmr.reset();

    oldPosition = newPosition;
    Serial.println("Position: " + String(newPosition));
    if ( _count_to_temp(newPosition) > MAX_SAFE_TEMP )
      newPosition = _temp_to_count(MAX_SAFE_TEMP);
    else if ( _count_to_temp(newPosition) < MIN_SAFE_TEMP )
      newPosition = _temp_to_count(MIN_SAFE_TEMP);
    _set_temp = _count_to_temp(newPosition);
  }
  else if ( _position_changing )
  {
    // Knob is not moving. Check to see if the movement has stopped.
    if ( _position_changing_tmr.expired() )
    {
      _position_changing = false;
      // Send updated set temperature (living_room_set_temp) to server.
      send_controller_set_temp("lr_temp", _set_temp);
      // Get the current temperature here. Send controller should have returned it, but
      // that isn't working for some reason.
      _set_temp = get_controller_set_temp("lr_temp");
      _temp_srvr_req_tmr.reset();
    }
  }

  // Retrieve the current set temperature from the server, but only if the user
  // isn't actively turning the knob.
  // @TODO: This will happen every 100 msec as currently written! Need to do it immediately
  // after a set operation, but only once every 5 seconds or so thereafter.
  if ( !_position_changing && _temp_srvr_req_tmr.expired() )
  {
    // Get the current temperature here.
    _set_temp = get_controller_set_temp("lr_temp");
    _temp_srvr_req_tmr.reset();
  }

  // Update the encoder position based on the new set temp.
  _encoder.setCount(_temp_to_count(_set_temp));

  // Update the display with the set temp
  if ( _display != nullptr )
  {
    _display->update(_set_temp);
  }

  return true;
} //update()