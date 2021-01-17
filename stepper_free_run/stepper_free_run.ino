// Paul Stoaks 11/11/2020
// This ESP32 sketch uses the ESP32 PWM to generate
// a constant string of pulses to spin a stepper motor.
// Encoder for input of RPM and direction, TFT display for output
//
// The timer code is based on the example in ESP-IDF 3.2.5, which I think
// is the version that the 1.0.5-rc6 Arduino ISP32 board definition is using.
// Note that this approach isn't really the "Arduino" approach so it may
// not work well.
//

#include "WiFi.h"
#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "esp_types.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

// NOTE:  The most recent version of the encoder from github is required
// for the ESP32: https://github.com/PaulStoffregen/Encoder
#include "Encoder.h"

// https://github.com/Bodmer/TFT_eSPI
// This video has a good summary of how to use the library, including the
// SD card.
// https://www.youtube.com/watch?v=rq5yPJbX_uk&ab_channel=XTronical
// User Setup comes from; TFT_eSPI/User_Setups/Pauls_ESP32_ILI9341.h
// May have to update TFT_eSPI/User_Setup_Select.h after a library edit to select this one.
#include "TFT_eSPI.h"

// Include our home-rolled WDT timer
#include "esp32_wdt.h"
#include "field.h"

//////////////////////////////////////////////
// WiFi Setup
/////////////////////////////////////////////
static const char SSID[] = "SSW3";
static const char PASSWD[] = "g0ne@gh0ti";
// here we set the static IP address:
static const IPAddress staticIp(192, 168, 0, 32);
static const IPAddress gatewayIp(192, 168, 0, 1);
static const IPAddress subnetMask(255, 255, 255, 0);

//////////////////////////////////////////////
// Pin Assignments
/////////////////////////////////////////////
// Encoder
static const unsigned ENC1_PB = 0;
static const unsigned ENC1_Q1 = 15;
static const unsigned ENC1_Q2 = 2;
// Stepper Motor
static const unsigned STEPS_PER_REV_MOTOR = 800; // Set via dipswitches
static const double   FINAL_DRIVE_RATIO = 1.8;
static const unsigned STEPS_PER_REV = static_cast<unsigned>(STEPS_PER_REV_MOTOR * FINAL_DRIVE_RATIO + 0.5);
static const unsigned SPINDLE_STEP_PIN = 12U;
static const unsigned SPINDLE_ENABLE_PIN = 13U;
static const unsigned SPINDLE_DIRECTION_PIN = 14U;
static const unsigned X_STEP_PIN = 25U;
static const unsigned X_ENABLE_PIN = 26U;
static const unsigned X_DIRECTION_PIN = 27U;
static const unsigned Y_STEP_PIN = 32U;
static const unsigned Y_ENABLE_PIN = 33U;
static const unsigned Y_DIRECTION_PIN = 16U;
static const unsigned INPUT_1 = 34; // Input only pin 1 Stopped working for some reason
static const unsigned EMERG_STOP = 35; // Input only pin 2 (button on breadboard)
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
// #define TFT_CS   22  // Chip select control pin
// #define TFT_DC   21  // Data Command control pin
// #define TFT_RST   4  // Reset pin (could connect to RST pin)
//#define TFT_RST  -1  // Set TFT_RST to -1 if display RESET is connected to ESP32 board RST



////////////////////////////////////////
// Display Code
////////////////////////////////////////
TFT_eSPI tft = TFT_eSPI();

////////////////////////////////////////
// STEPPER_CONTROLLER
////////////////////////////////////////
static float curr_dc = 30.0;               // Commanded duty cycle
static volatile unsigned target_freq = 0U; // Commanded frequency
static volatile unsigned curr_freq = 0U;   // Current frequency during accel/decel
static unsigned freq_accel = 2;            // Amount to increment requency at each timer cycle
// PWM_On tracks the inverse of the SPINDLE_ENABLE_PIN
// When PWM_On is true, SD will be low.
// When PWM_On is false, SD will be high, and the
// gate drive will be disabled.
volatile bool PWM_On = false;

////////////////////////////////////////
// Timer Code
// See: https://github.com/espressif/esp-idf/blob/2bfdd036b2dbd07004c8e4f2ffc87c728819b737/examples/peripherals/timer_group/main/timer_group_example_main.c
////////////////////////////////////////
static const unsigned TIMER_DIVIDER = 2U; // Hardware timer divider
static const unsigned TIMER_SCALE = TIMER_BASE_CLK / TIMER_DIVIDER;
static const timer_idx_t PULSE_FREQ_TMR = TIMER_0;
static const timer_idx_t UNUSED_TMR = TIMER_1;
//portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// Timer group0 ISR
// Starts pulse and computes the time for the next pulse.
// 
// Note:
// We don't call the timer API here because they are not declared with IRAM_ATTR.
// If we're okay with the timer irq not being serviced while SPI flash cache is disabled,
// we can allocate this interrupt without the ESP_INTR_FLAG_IRAM flag and use the normal API.
///
void IRAM_ATTR timer_group0_isr(void *para)
{
//   portENTER_CRITICAL_ISR(&timerMux);

   int timer_idx = (int) para;

   /* Retrieve the interrupt status and the counter value
      from the timer that reported the interrupt */
   uint32_t intr_status = TIMERG0.int_st_timers.val;
   TIMERG0.hw_timer[timer_idx].update = 1;
   uint64_t timer_counter_value =
      ((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
      | TIMERG0.hw_timer[timer_idx].cnt_low;


   /* Clear the interrupt
      and update the alarm time for the timer with without reload */
   if ( (intr_status & BIT(timer_idx)) && timer_idx == TIMER_0 ) {
      // Timer 0 interrupt
      TIMERG0.int_clr_timers.t0 = 1; // Clears the interrupt
      if ( PWM_On )
         gpio_set_level(static_cast<gpio_num_t>(SPINDLE_STEP_PIN), 1);

      if ( curr_freq < target_freq )
      {
         // Advance the frequency by the given amount
         curr_freq = ((curr_freq+freq_accel) < target_freq) ? curr_freq + freq_accel : target_freq;
      }
      else
      {
         curr_freq = target_freq; // Make it so, even though it alread sould be.
      }
      // The following calculation is slow. Would be better to be operating directly
      // on the period in TIMER_SCALE.
      timer_counter_value += static_cast<uint64_t>(1.0/curr_freq * TIMER_SCALE);
      TIMERG0.hw_timer[timer_idx].alarm_high = (uint32_t) (timer_counter_value >> 32);
      TIMERG0.hw_timer[timer_idx].alarm_low = (uint32_t) timer_counter_value;
      gpio_set_level(static_cast<gpio_num_t>(SPINDLE_STEP_PIN), 0);
   } else if ( (intr_status & BIT(timer_idx)) && timer_idx == TIMER_1 ) {
      // Timer 1 interrupt
      // Not using this timer at the moment
      TIMERG0.int_clr_timers.t1 = 1;
   } else {
      // Unexpected
   }

   /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
   TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

//   portEXIT_CRITICAL_ISR(&timerMux);
} // timer_group0_isr()

//
// Initialize selected timer of the timer group 0
//
// timer_idx - the timer number to initialize
// auto_reload - should the timer auto reload on alarm?
// period - Period
// TODO: Turn this around to use a countdown timer to avoid
//    eventual 64-bit overflow.
//
static void tg0_timer_init(timer_idx_t timer_idx,
                           bool auto_reload, unsigned period)
{
   // Select and initialize basic parameters of the timer
   timer_config_t config;
   config.divider = TIMER_DIVIDER;
   config.counter_dir = TIMER_COUNT_UP;
   config.counter_en = TIMER_PAUSE;
   config.alarm_en = TIMER_ALARM_EN;
   config.auto_reload = TIMER_AUTORELOAD_DIS;
   timer_init(TIMER_GROUP_0, timer_idx, &config);

   /* Timer's counter will initially start from value below.
      Also, if auto_reload is set, this value will be automatically reload on alarm */
   timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

   /* Configure the alarm value and the interrupt on alarm. */
   curr_freq = target_freq;
   timer_set_alarm_value(TIMER_GROUP_0, timer_idx, (uint64_t)(period * TIMER_SCALE));
   timer_enable_intr(TIMER_GROUP_0, timer_idx);
   timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                      (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

} // tg0_timer_init()

static void tg0_timer_start(timer_idx_t timer_idx)
{
   Serial.println("TimerStart");
   timer_start(TIMER_GROUP_0, timer_idx);
} // tg0_timer_start()

static void tg0_timer_pause(timer_idx_t timer_idx)
{
   Serial.println("TimerStop");
   timer_pause(TIMER_GROUP_0, timer_idx);
} // tg0_timer_pause()

////////////////////////////////////////
// MCPWM Code
////////////////////////////////////////
static const uint DUTY_CYCLE = 0.5;

// MCPWM Pins
//static const unsigned Q = 16U;     // GPIO16
//static const unsigned Q_BAR = 17U; // GPIO17
//static const unsigned SD = 18U;    // GPIO18 -- IR2110 shutdown pin (active high)

// static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};
#ifdef PWM
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
#endif // #ifdef PWM

static void set_duty_cycle(float dc)
{
#ifdef PWM
   // dc is the duty cycle, as a percentage (not a fraction.)
   // (e.g. 30.0 is 30%)
   curr_dc = dc;
   
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, dc);
//   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 100.0 - dc);
#endif
} // set_duty_cycle()

static void set_frequency(uint32_t freq)
{
   target_freq = freq;
} // Set the PWM frequency.

static void update_frequency()
{
#ifdef PWM
   // Gradually increases frequency until target frequency is reached.
   static const unsigned INCREMENT = 15U; // Amount frequency is increased each cycle.
#endif
   
   if ( !PWM_On )
     curr_freq = 0U;

   // It's OK that we set the frequency when
#ifdef PWM
   // the motor isn't running.
   int incr = target_freq - curr_freq;
   incr = (incr < 0) ? 0 : incr;  // Clamp at zero in case there is a bug.
   curr_freq += (incr <= INCREMENT) ? incr : INCREMENT;
   
   // freq is the desired frequency, in Hz.
   mcpwm_set_frequency(MCPWM_UNIT_0, MCPWM_TIMER_0, curr_freq*2U);

   // Reset duty_cycle after changing frequency.
   set_duty_cycle(curr_dc);
#endif
} // update_frequency()

////////////////////////////////////////
// Setup Encoder and Button Handler
////////////////////////////////////////

Encoder encoder1(ENC1_Q1, ENC1_Q2);

static volatile unsigned button_cnt = 0U; // Simple counter.  If greater than 0, button has been pressed.

void IRAM_ATTR buttonISR(void)
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
   RPM,        ///< Spindle RPM
   DIRECTION,  ///< Spindle direction
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
#ifdef PWM
   mcpwm_stop(MCPWM_UNIT_0, MCPWM_TIMER_0);
#endif
   PWM_On = false;
   
   reset_module(); // Defined in esp32_wdt.h
} // wdt_ISR()

////////////////////////////////////////
// Background Processing
////////////////////////////////////////
void setup()
{
   Serial.begin(115200);
   Serial.println("stepper_free_run.ino");

  // Configure static IP address
  if ( !WiFi.config(staticIp, gatewayIp, subnetMask) ) {
    Serial.println("STA Failed to configure");
  }
   
   WiFi.begin(SSID, PASSWD);
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.println("Connecting to WiFi..");
   }
   Serial.println("Connected to the WiFi network");
   
   pinMode(SPINDLE_STEP_PIN, OUTPUT);
   digitalWrite(SPINDLE_STEP_PIN, 0);
   pinMode(SPINDLE_ENABLE_PIN, OUTPUT);
   pinMode(SPINDLE_DIRECTION_PIN, OUTPUT);
   pinMode(EMERG_STOP, INPUT);
   pinMode(ENC1_PB, INPUT);
   setup_button_handler();
   
   tft.begin();
   tft.setRotation(0);
   tft.fillScreen(TFT_BLACK);            // Clear screen
#ifdef PWM   
   setup_mcpwm();
#else
   curr_freq = freq_accel; // Initialize frequency to first frequency.
   tg0_timer_init(PULSE_FREQ_TMR, false, 1.0/curr_freq);
#endif

   init_PWM_based_on_curr_field();

//   initialize_wdt(500, &wdt_ISR); // 500 msec

} // setup()

void loop()
{
   static int last_val_in_state = 0;
   static bool first_time_through_loop = true;

//   feed_wdt();

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
         Serial.println("LongPress");
       }
     }
     else
     {
       // Handle Short Press
       cycle_count = 0;
       if ( ++curr_field >= MAX_STATES )
         curr_field = 0;

       Serial.println("ShortPress");
//       Serial.println("Field: " + String(curr_field));

       encoder1.write(fields[curr_field].value());
       last_val_in_state = fields[curr_field].value();
     
       button_cnt = 0;
     }
   }

   // Check state of EMERG_STOP (limit switch?)
   // TODO: This should be an interrupt for safety
   PWM_On &= ( digitalRead(EMERG_STOP) == HIGH );
   // Serial.println(String("Emergency Stop: ") + String(digitalRead(EMERG_STOP)));
   
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

#if 0
   Serial.println("Field: " + String(curr_field)
              + " Value: " + String(fields[curr_field].value())
                  + " ButtonCnt: " + String(button_cnt));
#endif
   
   update_display();
   
   delay(200);

} // loop()

static const String get_direction_string(const int val)
{
   return val == CCW ? String("CCW") : String("CW");
} // get_direction_string()

static int enc_set_enable(bool val)
{
   static bool last_val = PWM_On;

   if ( val != last_val )
   {
      last_val = val;
      
      PWM_On = val;
      if ( val )
      {
         digitalWrite(SPINDLE_ENABLE_PIN, LOW);
         tg0_timer_start(PULSE_FREQ_TMR);
      }
      else
      {
         digitalWrite(SPINDLE_ENABLE_PIN, HIGH);
         tg0_timer_pause(PULSE_FREQ_TMR);
      }
//      Serial.println(PWM_On ? "Run" : "Stop");
   }

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

static unsigned get_rpm(int pct)
{
   static const unsigned MIN_RPM = 100;
   static const unsigned MAX_RPM = 1000;

   pct = pct/5;
   
   unsigned rpm = static_cast<unsigned>(((pct / 100.0) * (MAX_RPM - MIN_RPM)) + MIN_RPM);

   return rpm;
} // get_rpm()

static int enc_set_rpm(int val)
{
   // val is a percentage
   // Note that setting dy will change frequency because it changes the slope
   // and the number of steps per cycle is determined by the slope and the
   // amplitude.
   uint32_t rpm = get_rpm(val);
   
   unsigned new_freq = static_cast<unsigned>(rpm * STEPS_PER_REV / 60.0);

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
