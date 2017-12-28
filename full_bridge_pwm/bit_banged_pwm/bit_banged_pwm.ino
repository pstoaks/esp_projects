// Paul Stoaks 12/23/2017
// This sketch creates the two gate drive signals for a full bridge
// switching circuit.  Duty Cycle and ramp-rate (which affects frequency
// and duty cycle) are both adjustable with a rotary encoder.
// Compiled for a NodeMCU compatible board.

#include <ESP8266WiFi.h>
#include <Encoder.h>

////////////////////////////////////////
// Setup PWM.
////////////////////////////////////////
static const unsigned long INIT_DT = 2000U; // dt in ticks.  ISR fires with this period.
static const int INIT_TRIANGLE_AMPL = 2000; // Amplitude of triangle wave in counts.
static const int INIT_THRESHOLD = 800;      // Thresholds are symetric.  This should be positive.
                                            // Number of amplitude counts.
static const int INIT_DY = 20;              // Absolute value of dy (increment in amplitude with
											// each ISR call.
static const auto Q = D7;         // GPIO13
static const auto Q_BAR = D6;     // GPIO12
static const auto TIME_PIN = D5;  // GPIO14
static const unsigned DT = 21;    // dt in ticks.  ISR fires with this period. (base period is 5MHz)
static int triangle_amplitude;    // Amplitude of triangle wave in counts.
static int q_thresh;              // Threshold for Q output
static int q_bar_thresh;          // Threshold for Q_BAR output

// Variables updated by the interrupt.
static volatile int limit_counts;   // Peak of the triangle wave (amplitude/2)
static volatile int dy;             // Up and down dy in counts
static volatile bool rising = true; // Rising or falling slope
static volatile unsigned long next;
static volatile int ramp_cnt = 0;

// From esp8266_peri.h
#define GPOS   ESP8266_REG(0x304) //GPIO_OUT_SET WO register
#define GPOC   ESP8266_REG(0x308) //GPIO_OUT_CLR WO register
// The following can only be used for pins 0 through 15
#define SET_PIN(pin) (GPOS = (1 << (pin)))
#define CLR_PIN(pin) (GPOC = (1 << (pin)))

inline void switch_direction()
{
   rising = !rising;
   dy *= -1;
   limit_counts *= -1;
} // switch_direction()

void timerISR(void)
{
   unsigned clr = 0U;
   unsigned set = 0U;

   // Attach a scope to TIME_PIN to time this routine.
   SET_PIN(TIME_PIN);

   ramp_cnt += dy;

   if ( rising )
   {
	  if ( ramp_cnt > limit_counts )
	  {
		 switch_direction();
	  }
	  else
	  {
		 if ( ramp_cnt > q_thresh )
		 {
			set |= (1U << Q);
		 }
		 else
		 {
			clr |= (1U << Q);
		 }

		 if ( ramp_cnt > q_bar_thresh )
		 {
			clr |= (1U << Q_BAR);
		 }
		 else
		 {
			set |= (1U << Q_BAR);
		 }
	  }
   }
   else
   {
	  if ( ramp_cnt < limit_counts )
	  {
		 switch_direction();
	  }
	  else
	  {
		 if ( ramp_cnt < q_thresh )
		 {
			clr |= (1U << Q);
		 }
		 else
		 {
			set |= (1U << Q);
		 }

		 if ( ramp_cnt < q_bar_thresh )
		 {
			set |= (1U << Q_BAR);
		 }
		 else
		 {
			clr |= (1U << Q_BAR);
		 }
	  }
   }

   GPOC = clr;
   GPOS = set;
   
   CLR_PIN(TIME_PIN);

} // timerISR()

// Forward declaration
static void init_PWM_based_on_state(void);

void setup_timer1()
{
   // Output pins:
   pinMode(Q, OUTPUT);
   digitalWrite(Q, HIGH);
   pinMode(Q_BAR, OUTPUT);
   digitalWrite(Q_BAR, LOW);
   pinMode(TIME_PIN, OUTPUT);
   digitalWrite(TIME_PIN, LOW);

   // Set the parameters
   rising = true;
   dy = INIT_DY;
   triangle_amplitude = INIT_TRIANGLE_AMPL;
   limit_counts = triangle_amplitude/2;
   q_thresh = INIT_THRESHOLD;
   q_bar_thresh = q_thresh * -1;

   // Initialize the controlling parameters based on the state data
   init_PWM_based_on_state();

   noInterrupts();
   timer1_isr_init();
   timer1_attachInterrupt(timerISR);
   timer1_enable(TIM_DIV16, TIM_EDGE, TIM_LOOP);
   timer1_write(DT);
   interrupts();
} // setup_timer1()


////////////////////////////////////////
// Setup Encoder and Button Handler
////////////////////////////////////////

static const auto PB = D3;     // GPIO0
static const auto Q1 = D2;     // GPIO4
static const auto Q2 = D1;     // GPIO5

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

/////////////////////
// Setup
/////////////////////
void setup() 
{
   Serial.begin(115200);

   setup_timer1();
   Serial.println("Timer initialized.\n");

   setup_button_handler();
} // setup()


/////////////////////
// Loop
/////////////////////

enum States
{
   DUTY_CYCLE, ///< Encoder sets duty cycle
   DY,         ///< Encoder sets dy
   MAX_STATES
}; // enum States

static int values[MAX_STATES] = {50, 58};

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
	  case DUTY_CYCLE : values[state] = set_duty_cycle(values[state]);
		 break;
	  case DY : values[state] = set_dy(values[state]);
		 break;
	  } // switch(state)

	  last_val_in_state = values[state];
	  encoder.write(values[state]);
   }
   
   Serial.println("State: " + String(state)
				  + " Value: " + String(values[state]));

   delay(500);

   yield();
} // loop()

static inline int myabs(int v) { return (v >= 0) ? v : -1*v; }

static int set_duty_cycle(int val)
{
   // val is a percentage
   // Returns val
   static constexpr int MARGIN = {3}; // Margin at ends (1dy).  Don't allow 0 and 100%.
   static constexpr int MAX = {100 - MARGIN};
   static constexpr int MIN = {100 - MAX};

   // Clamp the value
   int pct = (val >= MIN) ? val : MIN;
   pct = (pct <= MAX) ? pct : MAX;

   // Reverse the percentage so that clockwise increases duty-cycle
   int p = 100 - pct;
   
   // Compute the controlling values.
   q_thresh = static_cast<int>((triangle_amplitude/2) * p/100.0);
   q_bar_thresh = q_thresh * -1;

   Serial.println("Q_Thresh: " + String(q_thresh));
	  
   return pct;
} // set_duty_cycle()

static int set_dy(int val)
{
   // val is a percentage.
   // Note that setting dy will change frequency because it changes the slope
   // and the number of steps per cycle is determined by the slope and the
   // amplitude.
   static constexpr int MAX = {100};
   static constexpr int MIN = {0};
   static constexpr int MIN_DY = {2};
   static constexpr int MAX_DY = {40};

   // Clamp the value
   int pct = (val >= MIN) ? val : MIN;
   pct = (pct <= MAX) ? pct : MAX;

   // Compute the new dy
   int new_dy = static_cast<int>((pct / 100.0) * (MAX_DY - MIN_DY)) + MIN_DY;

   Serial.println("dY: " + String(new_dy));

   // Turn off interrupts so that we can do it right.  We need to do a read to
   // determine whether dy is currently positive or negative before setting dy,
   // so we need to disable interrupts to ensure we don't get interrupted here.
   noInterrupts();
   {
	  if ( dy > 0 )
	  {
		 dy = new_dy;
	  }
	  else
	  {
		 dy = new_dy * -1;
	  }
   }
   interrupts();

   return pct;
} // set_dy()

static void init_PWM_based_on_state()
{
   values[DUTY_CYCLE] = set_duty_cycle(values[DUTY_CYCLE]);
   values[DY] = set_dy(values[DY]);
} // init_PWM
