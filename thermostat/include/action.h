// Action class

#pragma once

namespace SSW
{

class Action
{
public:
   Action() = default;
   virtual ~Action() = default;

   virtual bool execute() { return false; };
   virtual bool execute() const { return false; };
   virtual void reset() { };
}; // class Action

} // namespace SSW
