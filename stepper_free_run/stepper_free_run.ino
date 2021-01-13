// Paul Stoaks 11/11/2020
// This ESP32 sketch uses the ESP32 PWM to generate
// a constant string of pulses spin a stepper motor.
// Encoder for input of RPM and direction, TFT display for output
//

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"


// NOTE:  The most recent version of the encoder from github is required
// for the ESP32: https://github.com/PaulStoffregen/Encoder
#include "Encoder.h"

// https://github.com/Bodmer/TFT_eSPI
// This video has a good summary of how to use the library, including the
// SD card.
// https://www.youtube.com/watch?v=rq5yPJbX_uk&ab_channel=XTronical
#include "TFT_eSPI.h"

// Include our home-rolled WDT timer
#include "esp32_wdt.h"
#include "field.h"

//////////////////////////////////////////////
// Pin Assignments
/////////////////////////////////////////////
// Encoder
static const unsigned ENC1_PB = 0;
static const unsigned ENC1_Q1 = 15;
static const unsigned ENC1_Q2 = 2;
// Stepper Motor
static const unsigned STEPS_PER_REV = 800; // Set via dipswitches
static const unsigned SPINDLE_STEP_PIN = 12U;
static const unsigned SPINDLE_ENABLE_PIN = 13U;
static const unsigned SPINDLE_DIRECTION_PIN = 14U;
static const unsigned X_STEP_PIN = 25U;
static const unsigned X_ENABLE_PIN = 26U;
static const unsigned X_DIRECTION_PIN = 27U;
static const unsigned Y_STEP_PIN = 32U;
static const unsigned Y_ENABLE_PIN = 33U;
static const unsigned Y_DIRECTION_PIN = 16U;
static const unsigned INPUT_1 = 34; // Input only pin 1
static const unsigned INPUT_2 = 35; // Input only pin 2
static const unsigned INPUT_3 = 36; // Input only pin 3
static const unsigned INPUT_4 = 37; // Input only pin 4
static const unsigned INPUT_5 = 38; // Input only pin 5
static const unsigned INPUT_6 = 39; // Input only pin 6
// I2C
// SDA & SCL are defined for LoLin32 in pins_arduino.h
//static const unsigned SCL = 22;
//static const unsigned SDA = 21;

// Display  ILI9341
// See TFT_eSPI/User_Setup.h for pin definitions and other options
// For ESP32 Dev board (only tested with ILI9341 display)
// The hardware SPI can be mapped to any pins
// #define TFT_MISO 19
// #define TFT_MOSI 23
// #define TFT_SCLK 18
// #define TFT_CS   19  // Chip select control pin
// #define TFT_DC   21  // Data Command control pin
// #define TFT_RST   4  // Reset pin (could connect to RST pin)



////////////////////////////////////////
// Display Code
////////////////////////////////////////
TFT_eSPI tft = TFT_eSPI();

////////////////////////////////////////
// STEPPER_CONTROLLER
////////////////////////////////////////

////////////////////////////////////////
// MCPWM Code
////////////////////////////////////////
static const uint DUTY_CYCLE = 0.5;

// MCPWM Pins
//static const unsigned Q = 16U;     // GPIO16
//static const unsigned Q_BAR = 17U; // GPIO17
//static const unsigned SD = 18U;    // GPIO18 -- IR2110 shutdown pin (active high)

// static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

static void setup_mcpwm_pins()
{
   Serial.println("initializing mcpwm control gpio...n");
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, SPINDLE_STEP_PIN);
//   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, Q_BAR);
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
   // on pg. 299 of the reference manual (v2.2).  What is odd is that 'toggle' is not
   // supported.  I don't need these for my application, but I did experiment with them
   // and they work.
   // mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
   // mcpwm_set_signal_high(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
	
   // MCPWMXA to duty mode 1 and MCPWMXB to duty mode 0 or vice versa
   // will generate MCPWM compliment signal of each other, there are also
   // other ways to do it
   mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
//   mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_1);

   // Allows the setting of a minimum dead time between transitions to prevent
   // "shoot through" a totem-pole switch pair, which can be disastrous.
   mcpwm_deadtime_enable(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_ACTIVE_HIGH_MODE, 10, 10);   //Deadtime of 1uS

   mcpwm_start(MCPWM_UNIT_0, MCPWM_TIMER_0);
} // setup_mcpwm

// PWM State Variables
static float curr_dc = 30.0;
static volatile uint32_t target_freq = 0;
// PWM_On tracks the inverse of the SPINDLE_ENABLE_PIN
// When PWM_On is true, SD will be low.
// When PWM_On is false, SD will be high, and the
// gate drive will be disabled.
volatile bool PWM_On = false;

static void set_duty_cycle(float dc)
{
   // dc is the duty cycle, as a percentage (not a fraction.)
   // (e.g. 30.0 is 30%)
   curr_dc = dc;
   
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dc);
//   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 100.0 - dc);
} // set_duty_cycle()

static void set_frequency(uint32_t freq)
{
   target_freq = freq;
} // Set the PWM frequency.

static unsigned curr_freq = 0U;

static void update_frequency()
{
   // Gradually increases frequency until target frequency is reached.
   static const unsigned INCREMENT = 15U; // Amount frequency is increased each cycle.
   
   if ( !PWM_On )
	  curr_freq = 0U;

   // It's OK that we set the frequency when
   // the motor isn't running.
   int incr = target_freq - curr_freq;
   incr = (incr < 0) ? 0 : incr;  // Clamp at zero in case there is a bug.
   curr_freq += (incr <= INCREMENT) ? incr : INCREMENT;
   
   // freq is the desired frequency, in Hz.
   mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, curr_freq*2U);

   // Reset duty_cycle after changing frequency.
   set_duty_cycle(curr_dc);
} // update_frequency()

////////////////////////////////////////
// Setup Encoder and Button Handler
////////////////////////////////////////

Encoder encoder1(ENC1_Q1, ENC1_Q2);

static volatile unsigned button_cnt = 0U; // Simple counter.  If greater than 0, button has been pressed.

void buttonISR(void)
{
   button_cnt++;
} // buttonISR()

void setup_button_handler()
{
   button_cnt = 0U;
   attachInterrupt(ENC1_PB, buttonISR, FALLING);
} // setup_button_handler()

// Forward declaration
static void init_PWM_based_on_curr_field(void);

// Fields in the menu
enum Fields
{
   NULL_STATE, ///< A state where nothing happens.
   RPM,       ///< Spindle RPM
   DIRECTION, ///< Spindle direction
   MAX_STATES
}; // enum Fields

static const int CW  = 0U;
static const int CCW = 1U;

// Fields array.
static SSW::Field fields[MAX_STATES] = {
   { "NULL", 0, 100, 0, false },
   { "RPM", 0, 500, 0, false},
   { "Dir", 0, 1, CW, true }
}; // fields array
static volatile unsigned curr_field = 1U;

void update_display()
{
   // Updates the display.  Value currently being adjusted blinks.
   static const int LEFT_MARGIN = 6;
   static const int TOP_MARGIN = 18;
   static const int SCREEN_WIDTH = 240;
   static const int SCREEN_HEIGHT = 320;
   
   static unsigned cntr = {0U};
//   tft.fillScreen(TFT_BLACK);            // Clear screen

   tft.setFreeFont(&FreeSans18pt7b);        // Select the font
   tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Yellow over black
//   tft.setTextPadding(20);
   int line_height = tft.fontHeight();
   
   // Only the lines enabled in this cycle are displayed.
   // Lines for selected adjustment are only displayed every other
   // cycle.

   int line = TOP_MARGIN;
   tft.setTextDatum(TL_DATUM);

   // If the RPM is selected for changing, blink it
   if (curr_field == RPM)
   {
	  if ( cntr % 2 )
	  {
		 // RPM is drawn centered.
		 tft.setTextDatum(MC_DATUM);
		 tft.drawString(String(get_rpm(fields[RPM].value())),
						SCREEN_WIDTH/2, line + line_height/2);
	  }
	  else // Blank the line
		 tft.fillRect(LEFT_MARGIN, line, SCREEN_WIDTH, line_height, TFT_BLACK);
   }
   else
   {
	  // RPM is drawn centered.
	  tft.setTextDatum(MC_DATUM);
	  tft.drawString(String(get_rpm(fields[RPM].value())),
					 SCREEN_WIDTH/2, line + line_height/2);
   }
   line += line_height;
   
   // If the direction is selected for changing, display it
   if ( curr_field == DIRECTION)
   {
	  if (cntr % 2)
	  {
		 tft.setTextDatum(TL_DATUM);
		 tft.drawString(fields[DIRECTION].name() + ": " + get_direction_string(fields[DIRECTION].value()),
						LEFT_MARGIN, line);
	  }
	  else // Blank the line
		 tft.fillRect(LEFT_MARGIN, line, SCREEN_WIDTH, line_height, TFT_BLACK);
   }
   else
   {
	  tft.setTextDatum(TL_DATUM);
	  tft.drawString(fields[DIRECTION].name() + ": " + get_direction_string(fields[DIRECTION].value()),
					 LEFT_MARGIN, line);
   }
   line += line_height;
   
   tft.setTextDatum(MC_DATUM);
   // Blank the line (every time, causes some flashing)
   tft.fillRect(LEFT_MARGIN, line, SCREEN_WIDTH, line_height, TFT_BLACK);
   if ( PWM_On )
	  tft.drawString("RUN",
					 SCREEN_WIDTH/2, line + line_height/2);
   else
	  tft.drawString("STOP",
					 SCREEN_WIDTH/2, line + line_height/2);	  
   line += line_height;

   line += line_height;
   tft.fillRect(LEFT_MARGIN, line, SCREEN_WIDTH, line_height, TFT_BLACK);
   tft.drawString(String(curr_freq),
				  SCREEN_WIDTH/2, line + line_height/2);
   

   cntr++;
} // update_display()

////////////////////////////////////////
// WDT ISR
////////////////////////////////////////
void IRAM_ATTR wdt_ISR()
{
   // Force the PWM into a safe curr_field immediately.
   digitalWrite(SPINDLE_ENABLE_PIN, HIGH);
   mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
   PWM_On = false;
   
   reset_module(); // Defined in esp32_wdt.h
} // wdt_ISR()

////////////////////////////////////////
// Background Processing
////////////////////////////////////////
void setup()
{
   pinMode(SPINDLE_ENABLE_PIN, OUTPUT);
   pinMode(SPINDLE_DIRECTION_PIN, OUTPUT);
   pinMode(INPUT_1, INPUT);
   
   Serial.begin(115200);
   Serial.println("Hello!");

   tft.begin();
   tft.setRotation(0);
   tft.fillScreen(TFT_BLACK);            // Clear screen
   
   setup_mcpwm();

   init_PWM_based_on_curr_field();

   initialize_wdt(500, &wdt_ISR); // 500 msec

   setup_button_handler();
} // setup()

void loop()
{
   static int last_val_in_state = 0;
   static bool first_time_through_loop = true;

   feed_wdt();

   if ( first_time_through_loop )
   {
	  // Initialization
	  encoder1.write(fields[curr_field].value());
	  first_time_through_loop = false;
	  last_val_in_state = fields[curr_field].value();
   }

   // Handle the button press to change states.
   if ( button_cnt > 0 )
   {
	  static unsigned cycle_count = 0;
	  // If button has been pressed and held down for
	  // two cycles, change the PWM_On curr_field.
	  if ( (curr_field == NULL_STATE) && (digitalRead(ENC1_PB) == LOW) )
	  {
		 // Handle Long Press
		 cycle_count++;
		 if ( cycle_count > 1 )
		 {
			// Long Press occurred.
			PWM_On = ! PWM_On;
			cycle_count = 0;
			button_cnt = 0;
		 }
	  }
	  else
	  {
		 // Handle Short Press
		 cycle_count = 0;
		 if ( ++curr_field >= MAX_STATES )
			curr_field = 0;

//		 Serial.println("Field: " + String(curr_field));

		 encoder1.write(fields[curr_field].value());
		 last_val_in_state = fields[curr_field].value();
	  
		 button_cnt = 0;
	  }
   }

   // Check state of INPUT_1 (limit switch?)
   PWM_On &= ( digitalRead(INPUT_1) == HIGH );
   
   // React to whatever changes have been made.
   auto position = encoder1.read();
   if ( position != last_val_in_state )
   {
	  switch(curr_field)
	  {
	  default:
	  case NULL_STATE:
		 break;
	  case DIRECTION :
		 static auto direction = CW;
		 if (++direction > 1)
			direction = CW;
		 enc_set_direction(direction);
         break;
      case RPM :
		 enc_set_rpm(fields[curr_field].set_value(position));
		 break;
	  } // switch(curr_field)

	  last_val_in_state = position;
   }

   // Enable or disable the motor
   enc_set_enable(PWM_On);
   
   // Update the frequency
   update_frequency();

//   Serial.println("Field: " + String(curr_field)
//				  + " Value: " + String(fields[curr_field].value()));

   update_display();
   
   delay(200);

} // loop()

static const String get_direction_string(const int val)
{
   return val == CCW ? String("CCW") : String("CW");
} // get_direction_string()

static int enc_set_enable(bool val)
{
   PWM_On = val;
   if ( val )
	  digitalWrite(SPINDLE_ENABLE_PIN, LOW);
   else
	  digitalWrite(SPINDLE_ENABLE_PIN, HIGH);

   return val;
} // enc_set_enable()

static int enc_set_direction(int val)
{
   // val is CW or CCW
   // Returns val

   val = fields[DIRECTION].set_value(val);
   // For safety, shut the motor down.
   enc_set_enable(false);
   
   // Compute the controlling fields.
   if (val == CCW)
   {
	  digitalWrite(SPINDLE_DIRECTION_PIN, LOW);
   }
   else
   {
	  digitalWrite(SPINDLE_DIRECTION_PIN, HIGH);
   }
   Serial.println("Direction: " + get_direction_string(val));
	  
   return val;
} // enc_set_direction()

static uint32_t get_rpm(int pct)
{
   static const unsigned MIN_RPM = 100;
   static const unsigned MAX_RPM = 3000;

   pct = pct/5;
   
   uint32_t rpm = static_cast<uint32_t>(((pct / 100.0) * (MAX_RPM - MIN_RPM)) + MIN_RPM);

   return rpm;
} // get_rpm()

static int enc_set_rpm(int val)
{
   // val is a percentage
   // Note that setting dy will change frequency because it changes the slope
   // and the number of steps per cycle is determined by the slope and the
   // amplitude.
   uint32_t rpm = get_rpm(val);
   
   uint32_t new_freq = static_cast<uint32_t>(rpm * STEPS_PER_REV / 60.0);

   Serial.println("RPM: " + String(rpm) + " Freq: " + String(new_freq));

   set_frequency(new_freq);
   
   return rpm;
} // enc_set_rpm()

static void init_PWM_based_on_curr_field()
{
   enc_set_direction(fields[DIRECTION].value());
   set_duty_cycle(curr_dc);
   enc_set_rpm(fields[RPM].value());
   enc_set_enable(PWM_On);
} // init_PWM
