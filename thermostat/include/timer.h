#pragma once

#include <cstdint>

#include <Arduino.h>

#include "action.h"

namespace SSW
{

// Some useful constants
constexpr uint32_t secInMicroSec = {1000000U};   // 1 second expressed in
constexpr uint32_t secInMilliSec = {1000U};      // 1 second expressed in milliseconds

/// @todo
/// This would make a great template, with the action as the 
/// type.  That way, we wouldn't have to pass an action in.
class Timer
{
public:

   Timer(uint32_t msec_time, Action *action=nullptr) : 
      _millis_time(msec_time),
      _expiry(0),
      _action(action)
   {
   }
   ~Timer() { _action = nullptr; _expiry = 0; _millis_time=0; }

   void reset() 
   { 
      if ( _action != nullptr )
         _action->reset(); 
      _expiry = millis() + _millis_time;
   }

   /// @brief
   /// Sets the timeout period.  Takes effect
   /// on the next reset().
   void set_timeout(uint32_t msec_time)
   {
	  _millis_time = msec_time;
   }

   operator bool() { return !expired(); }

   bool expired() 
   { 
      // Note that action is executed every time this is called, not just the
      // first time.  Action can be made a one-shot internally, if that is the
      // behavior desired.
      if ( millis() >= _expiry )
      {
         if ( _action != NULL )
            _action->execute();
         return true;
      }
      else
         return false;
   } // expired()

   Action* action() { return _action; }
   Action const * action() const { return _action; }

private:
   uint32_t _millis_time; // Timeout time.
   uint32_t _expiry; // Time, in milliseconds, that timer will expire
   Action  *_action; // The optional action class.  Called when timer expires.

}; // Class Timer

} // namespace SSW
