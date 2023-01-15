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

  /// @brief Initialize the controller.
  /// @return None
  virtual void init() = 0;

  /// @brief Called from loop() Performs required update operations.
  /// @return true for success.
  virtual bool update() = 0;


}; // class ControllerIfc

// Timeout period that determines when the user has stopped turning the
// temperature adjustment knob.
static constexpr unsigned POSITION_CHANGING_MS = 2000;

// Input temperature values are clamped to the following values.
static constexpr unsigned MIN_SAFE_TEMP = 50.0; // Minimum safe temp in encoder counts.
static constexpr unsigned MAX_SAFE_TEMP = 75.0; // Maximum safe temp in encoder counts.

/// @class TempController
///
/// @brief
/// This controller manages sycnhronization between a
/// locally stored temperature member and a server. It has
///  an optoencoder that provides input.
/// @param update_period_ms: Time between encoder checks.
/// @param srvr_req_period_ms: Time between server calls to update set temp.
/// @param encoder: The rotary encoder used to set the temperature.
/// @param init_set: The initial set temp
/// @param mult: The multiplier to apply to the rotary encoder (ticks per detent)
/// @param controller_name: Controller name on server (e.g. "lr_temp"). This has to be a constant string.
/// @param display: The display element for displaying the set temperature value.
/// @remarks None
class TempController: public ControllerIfc
{
public:
  TempController(unsigned update_period_ms, unsigned srvr_req_period_ms,
    ESP32Encoder& encoder, float init_set, float mult, const char* controller_name,
    DisplayElemIfc* const display = nullptr) :
    _set_temp(init_set),
    _encoder(encoder),
    _multiplier(mult),
    _display(display),
    _upd_timer(update_period_ms),
    _temp_srvr_req_tmr(srvr_req_period_ms),
    _controller_name(controller_name)
    {}
  virtual ~TempController() override {};

  // @brief Initializes the controller. Internally, the encoder is given an initial
  // value.
  virtual void init() override;

  virtual bool update() override;

  float set_temp() const { return _set_temp; }

protected:
  float _set_temp; ///< The current set temp for the controller
  ESP32Encoder& _encoder; ///< The rotary encoder being used.
  float _multiplier; ///< Ticks per detent
  DisplayElemIfc* const _display; ///< The set temp display element
  SSW::Timer _upd_timer;  ///< The encoder read timer
  bool _position_changing {false}; ///< True if the position is currently being changed.
  SSW::Timer _position_changing_tmr { POSITION_CHANGING_MS }; ///< Timeout to accept encoder position as set position (rotation has stopped)
  SSW::Timer _temp_srvr_req_tmr; ///< The server request timer to update set_temp from the server.
  const char* _controller_name; ///< The controller name on the server.

private:
  /// @brief Returns the encoder counts associated with the input temperature
  /// @param t: Temperature to be converted to counts
  /// @return /// Encoder counts associated with the input temperature
  inline int64_t _temp_to_count(float t)
  {
    return static_cast<int64_t>(t*_multiplier+0.5);
  } // temp_to_count()

  /// @brief Returns the temperature represented by the count rounded to tenths.
  /// @param cnt: The encoder count to convert
  /// @return The temperature represented by the count rounded to tenths.
  inline float _count_to_temp(int64_t cnt)
  {
    return static_cast<float>(static_cast<int>(cnt/_multiplier*10.0+0.5)/10.0);
  } // _count_to_temp()

}; // class TempController

