/////////////////////////////////////////////////////////////////////
/// ESP32 PWM class
/// Paul Stoaks
/// Nov. 22, 2020
///
/// Sets up PWM on a pin and allows for some control.
///
/// MCPWM API is documented at:
/// http://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/mcpwm.html
/////////////////////////////////////////////////////////////////////

#pragma once

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

namespace SSW
{

class ESP32_PWM
{
public:
   ESP32_PWM(unsigned pin) :
	  m_pin {pin};
   
   ~ESP32_PWM() {
	  stop();
	  disconnect();
   } // dtor()

   // Initialize the controller. This actually takes control of the output pin.
   bool init(bool enable_deadtime = false);

   // Disconnects the controller. Pin is now available for some other use and
   // timers are shut off.
   void disconnect(void);

   // Set the duty cycle. dc is a percentage.
   void set_duty_cycle(float dc);
   float duty_cycle() const { return m_dc; }

   // Set the frequency.
   void set_freq(unsigned freq);
   unsigned freq() const { return m_freq; }

   // Start and stop the PWM
   void start();
   void stop();
   
private:
   unsigned m_pin; ///< The output pin
   float    m_dc;  ///< The currently set duty cycle
   unsigned m_freq; ///< The currently set frequency

   ESP32_PWM() = delete;
   ESP32_PWM(const Field &) = delete;
   ESP32_PWM& operator=(const Field &) = delete;
   
}; // ESP32_PWM

}; // namespace
