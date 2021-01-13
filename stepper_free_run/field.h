#pragma once

namespace SSW
{

class Field
{
public:
   Field(String const& name,
		 int min_value,
		 int max_value,
	     int initial_val,
	     bool wrap = false) :
	  m_value { initial_val },
	  m_name {name},
	  m_min_v { min_value },
	  m_max_v { max_value },
	  m_wrap  { wrap }
   {};
	  
   ~Field() {};

   String const& name() const { return m_name; }
   
   int value() const { return m_value; }
   int set_value(int val)
   {
	  if ( !m_wrap )
	  {
		 // Clamp to min/max
		 if ( val < m_min_v )
			val = m_min_v;
		 else if ( val > m_max_v)
			val = m_max_v;
	  }
	  else
	  {
		 // Wrap around
		 if ( val < m_min_v )
			val = m_max_v;
		 else if ( val > m_max_v)
			val = m_min_v;
	  }
	  m_value = val;
	  
	  return val;
   } // set_value()

   String toString() const
   {
	  return m_name + ": " + String(m_value);
   } // toString()

private:
   int m_value;
   String m_name;
   int m_min_v;
   int m_max_v;
   bool m_wrap;

   Field() = delete;
   Field(const Field &) = delete;
   Field& operator=(const Field &) = delete;
}; // class Field
}; // namespace
