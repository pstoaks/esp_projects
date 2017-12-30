// Paul Stoaks 12/28/2017
// This ESP32 sketch creates the two gate drive signals for a full bridge
// switching circuit.  Duty Cycle and frequency are both
// adjustable with a rotary encoder.
// Compiled for a WEMOS LoLin32 board, but should work with any ESP32.
//
// As always, we stand on the shoulders of giants.  The following is hacked
// together from:
// https://github.com/espressif/esp-idf/blob/7d0d2854d34844c7cfc49fc0604287c5e033f00b/examples/peripherals/mcpwm/mcpwm_bldc_control/main/mcpwm_bldc_control_hall_sensor_example.c

// MCPWM API is documented at:
// http://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/mcpwm.html

// ESP32 Reference Manual: http://espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf


#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

// NOTE:  The most recent version of the encoder from github is required
// for the ESP32: https://github.com/PaulStoffregen/Encoder
#include "Encoder.h"

// MCPWM Pins
static const unsigned Q = 16;     // GPIO16;
static const unsigned Q_BAR = 17; // GPIO17;

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

static void setup_mcpwm_pins()
{
   Serial.println("initializing mcpwm control gpio...n");
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, Q);
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, Q_BAR);
} // setup_pins()

static void setup_mcpwm()
{
   setup_mcpwm_pins();

   Serial.println("Configuring initial parameters ...\n");
   // Not that this is initial configuration only, further, more
   // detailed configuration will often be required.
   mcpwm_config_t pwm_config;
   pwm_config.frequency = 10000;  //frequency = 1000Hz
   pwm_config.cmpr_a = 30.0;      //duty cycle of PWMxA = 30.0%
   pwm_config.cmpr_b = 30.0;      //duty cycle of PWMxB = 30.0%
   pwm_config.counter_mode = MCPWM_UP_DOWN_COUNTER; // Up-down counter (triangle wave)
   pwm_config.duty_mode = MCPWM_DUTY_MODE_0; // Active HIGH
   mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    //Configure PWM0A & PWM0B with above settings

   // The following calls set what happens on transitions relative to the
   // comparators.  The API documentation doesn't make this clear, but
   // I believe that these set the 'actions' described under "PWM Signal Generation"
   // in section 15.3.3 on pg. 341 of the reference manual (v2.7).  What is odd is
   // that 'toggle' is not supported.  I don't need these for my application, but
   // I did experiment with them and they work.
   // mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
   // mcpwm_set_signal_high(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
	
   // MCPWMXA to duty mode 1 and MCPWMXB to duty mode 0 or vice versa
   // will generate MCPWM compliment signal of each other, there are also
   // other ways to do it
   mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
   mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_1);

   // Allows the setting of a minimum dead time between transitions to prevent
   // "shoot through" a totem-pole switch pair, which can be disastrous.
//	mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_BYPASS_FED, 100, 100);   //Deadtime of 10us

   mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
} // setup_mcpwm

static void set_duty_cycle(float dc)
{
   // dc is the duty cycle, as a percentage (not a fraction.)
   // (e.g. 30.0 is 30%)
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dc);
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 100.0 - dc);
} // set_duty_cycle()

static void set_frequency(uint32_t freq)
{
   // freq is the desired frequency, in Hz.
   mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, freq*2U);
} // Set the PWM frequency.

////////////////////////////////////////
// Setup Encoder and Button Handler
////////////////////////////////////////

static const auto PB = 0;      // GPIO0
static const auto Q1 = 15;     // GPIO15
static const auto Q2 = 2;      // GPIO2

Encoder encoder(Q1, Q2);

static volatile unsigned button_cnt = 0U; // Simple counter.  If greater than 0, button has been pressed.

void buttonISR(void)
{
   button_cnt++;
} // buttonISR()

void setup_button_handler()
{
   button_cnt = 0U;
   attachInterrupt(PB, buttonISR, FALLING);
} // setup_button_handler()


// Forward declaration
static void init_PWM_based_on_state(void);

void setup()
{
   Serial.begin(115200);
   Serial.println("Hello!");

   setup_mcpwm();

   init_PWM_based_on_state();

   setup_button_handler();
} // setup()

enum States
{
   DUTY_CYCLE, ///< Encoder sets duty cycle
   FREQ,       ///< Encoder sets frequency
   MAX_STATES
}; // enum States

static int values[MAX_STATES] = {50, 50}; // Initialized duty cycle and frequency to 50%

void loop()
{
   static unsigned state = 0U;
   static int last_val_in_state = 0;
   static bool first_time_through_loop = true;

   if ( first_time_through_loop )
   {
	  encoder.write(values[state]);
	  first_time_through_loop = false;
	  last_val_in_state = values[state];
   }

   values[state] = encoder.read();

   if ( button_cnt > 0 )
   {
	  // button has been pressed
	  state++;
	  if (state > MAX_STATES-1)
		 state = 0;

	  encoder.write(values[state]);
	  last_val_in_state = values[state];
	  
	  button_cnt = 0;
   }

   // React to whatever changes have been made.
   if ( values[state] != last_val_in_state )
   {
	  switch(state)
	  {
	  case DUTY_CYCLE : values[state] = enc_set_dc(values[state]);
		 break;
	  case FREQ : values[state] = enc_set_freq(values[state]);
		 break;
	  } // switch(state)

	  last_val_in_state = values[state];
	  encoder.write(values[state]);
   }
   
   Serial.println("State: " + String(state)
				  + " Value: " + String(values[state]));

   delay(500);

} // loop()

static int enc_set_dc(int val)
{
   // val is a percentage
   // Returns val
   static constexpr int MAX = {100};
   static constexpr int MIN = {10};

   // Clamp the value
   int pct = (val >= MIN) ? val : MIN;
   pct = (pct <= MAX) ? pct : MAX;

   // Compute the controlling values.
   set_duty_cycle(pct/2.0);
   Serial.println("Duty Cycle: " + String(pct/2.0));
	  
   return pct;
} // enc_set_dc()

static int enc_set_freq(int val)
{
   // val is a percentage.
   // Note that setting dy will change frequency because it changes the slope
   // and the number of steps per cycle is determined by the slope and the
   // amplitude.
   static constexpr int MAX = {100};
   static constexpr int MIN = {0};
   static constexpr uint32_t MIN_FREQ = {500};
   static constexpr uint32_t MAX_FREQ = {10000};

   // Clamp the value
   int pct = (val >= MIN) ? val : MIN;
   pct = (pct <= MAX) ? pct : MAX;

   // Compute the new frequency
   int new_freq = static_cast<int>((pct / 100.0) * (MAX_FREQ - MIN_FREQ)) + MIN_FREQ;

   Serial.println("Freq: " + String(new_freq));

   set_frequency(new_freq);
   
   return pct;
} // enc_set_freq()

static void init_PWM_based_on_state()
{
   values[DUTY_CYCLE] = enc_set_dc(values[DUTY_CYCLE]);
   values[FREQ] = enc_set_freq(values[FREQ]);
} // init_PWM
