#pragma once

#include "timer.h"

class DisplayElemIfc;
class ESP32Encoder;

/// @class ControllerIfc
///
/// @brief
/// Provides an interface for controllers. A controller has state, 
/// some kind of input from the loop, and an optional LVGL output
/// field.
///
/// @remarks
class ControllerIfc
{
public:
  virtual ~ControllerIfc() {};

  /// @brief Called from loop() Performs required update operations.
  /// @return true for success.
  virtual bool update() = 0;


}; // class ControllerIfc

// Timeout period that determines when the user has stopped turning the
// temperature adjustment knob.
static constexpr unsigned POSITION_CHANGING_MS = 2000;

// Input temperature values are clamped to the following values.
static constexpr unsigned MIN_SAFE_TEMP = 500; // Minimum safe temp in encoder counts.
static constexpr unsigned MAX_SAFE_TEMP = 750; // Maximum safe temp in encoder counts.

/// @class TempController
///
/// @brief
/// This controller manages sycnhronization between a
/// locally stored temperature member and a server. It has
//  an optoencoder that provides input.
/// @remarks
class TempController: public ControllerIfc
{
public:
  TempController(unsigned update_period_ms, unsigned srvr_req_period_ms,
    ESP32Encoder& encoder, DisplayElemIfc* const display = nullptr) :
    _encoder(encoder),
    _display(display),
    _upd_timer(update_period_ms),
    _temp_srvr_req_tmr(srvr_req_period_ms)
    {}
  virtual ~TempController() override {};

  virtual bool update() override;

protected:
  float _set_temp {68.0};
  ESP32Encoder& _encoder;
  DisplayElemIfc* const _display;
  SSW::Timer _upd_timer;
  bool _position_changing {false}; ///<< True if the position is currently being changed.
  SSW::Timer _position_changing_tmr { POSITION_CHANGING_MS };
  SSW::Timer _temp_srvr_req_tmr;
}; // class TempController

