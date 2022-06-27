#include <Arduino.h>

////////////////////////////////////////////////
// Encoder
#include <ESP32Encoder.h>

static const unsigned ENC1_PB = 27;
static const unsigned ENC1_Q1 = 26;
static const unsigned ENC1_Q2 = 25;

// https://github.com/Bodmer/TFT_eSPI
// This video has a good summary of how to use the library, including the
// SD card.
// https://www.youtube.com/watch?v=rq5yPJbX_uk&ab_channel=XTronical
// User Setup comes from; TFT_eSPI/User_Setups/Pauls_ESP32_ILI9341.h
// May have to update TFT_eSPI/User_Setup_Select.h after a library edit to select this one.
#include <TFT_eSPI.h>

////////////////////////////////////////
// Display Code
// Note that pinout and other parameters are defined in library
// User_Setup.h
////////////////////////////////////////

TFT_eSPI tft = TFT_eSPI();


static const int LEFT_MARGIN = 6;
static const int TOP_MARGIN = 18;
static const int SCREEN_WIDTH = TFT_HEIGHT;
static const int SCREEN_HEIGHT = TFT_WIDTH;

////////////////////////////////////////
// Setup Encoder and Button Handler
////////////////////////////////////////
ESP32Encoder encoder;

static volatile unsigned button_cnt = 0U; // Simple counter.  If greater than 0, button has been pressed.

void buttonISR(void)
{
   button_cnt++;
} // buttonISR()

void setup_button_handler()
{
   button_cnt = 0U; // file static volatile
   pinMode(ENC1_PB, INPUT_PULLUP);
   attachInterrupt(ENC1_PB, buttonISR, FALLING);
} // setup_button_handler()

// Include our home-rolled WDT timer
#include "esp32_wdt.h"

void IRAM_ATTR wdt_ISR()
{
   // Put device into a safe state, then reset.
   
   reset_module(); // Defined in esp32_wdt.h
} // wdt_ISR()


/////////////////////////////////////////////
// DHT22 Temp & Humidity sensor
#include "DHT.h"

static const unsigned DHTPIN = 33;

// Uncomment whatever type you're using!
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Initialize DHT sensor.
// Note that older versions of this library took an optional third parameter to
// tweak the timings for faster processors.  This parameter is no longer needed
// as the current DHT reading algorithm adjusts itself to work on faster procs.

DHT dht(DHTPIN, DHTTYPE);


/////////////////////////////////////////////////
// Light sensor pin
static const unsigned LIGHT_PIN = 36; // ADC1_0

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Hello World!");

 	//ESP32Encoder::useInternalWeakPullResistors=DOWN;
	// Enable the weak pull up resistors
	ESP32Encoder::useInternalWeakPullResistors=UP;
	encoder.attachHalfQuad(ENC1_Q1, ENC1_Q2);
  encoder.setCount(680); // 68.0 degrees, fixed point

  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);            // Clear screen

  tft.setFreeFont(&FreeSans18pt7b);        // Select the font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Yellow over black
  

  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();

  initialize_wdt(500, &wdt_ISR); // 500 msec

  setup_button_handler();
} // setup()

void loop() {
  static unsigned loop_cntr = 0;
  static const unsigned DELAY = 50;

  feed_wdt();

  // Only the lines enabled in this cycle are displayed.
  // Lines for selected adjustment are only displayed every other
  // cycle.

  int line = TOP_MARGIN;
  
  static int line_height = tft.fontHeight();
  tft.setTextDatum(TL_DATUM);

  tft.setTextDatum(MC_DATUM);
  tft.drawString("Hello",
            SCREEN_WIDTH/2, line + line_height/2);

  line += line_height;

  tft.drawString("World",
            SCREEN_WIDTH/2, line + line_height/2);

  line += line_height;

  // LED Blink Code
  if (loop_cntr % (1000/DELAY) == 0) // 1 second
  {
    static bool on = false;
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);   // turn the LED on (HIGH is the voltage level)
    on = !on;
  }

  if (loop_cntr % (2000/DELAY) == 0)
  {
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float h = dht.readHumidity();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    float f = dht.readTemperature(true);

    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(f)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else
    {
      // Compute heat index in Fahrenheit (the default)
      float hif = dht.computeHeatIndex(f, h);

      Serial.print(F("Humidity: "));
      Serial.print(h);
      Serial.print(F("%  Temperature: "));
      Serial.print(f);
      Serial.print(F("°F  Heat index: "));
      Serial.print(hif);
      Serial.println(F("°F"));
    }
  }

  if (loop_cntr % (100/DELAY) == 0)
  {

    static unsigned long oldPosition  = encoder.getCount();

    unsigned long newPosition = encoder.getCount();
    if (newPosition != oldPosition) {
      oldPosition = newPosition;
      Serial.println("Position: " + String(newPosition));
    }
  }

  if ((loop_cntr % (500/DELAY) == 0) && !digitalRead(ENC1_PB))
  {
    Serial.println("Pressed : Button Cnt: " + String(button_cnt));
  }

  if (loop_cntr % (500/DELAY) == 0)
  {
    // Read light level
    auto light_level = analogRead(LIGHT_PIN);
    Serial.println("Light: " + String(light_level));
  }

  loop_cntr++;
  delay(DELAY);
} // loop()
