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
//
// User Setup comes from; TFT_eSPI/User_Setups/User_Setup.h
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <SD.h>
#include <FS.h>

////////////////////////////////////////
// Note that pinout and other parameters are defined in library
// User_Setup.h
////////////////////////////////////////

static const int LEFT_MARGIN = 6;
static const int TOP_MARGIN = 18;
static const int SCREEN_WIDTH = TFT_HEIGHT;
static const int SCREEN_HEIGHT = TFT_WIDTH;
static const unsigned TFT_BACKLIGHT = 5U;
static const unsigned SD_CS = 13U;
static uint16_t TOUCH_CAL_DATA[5] = { 275, 3620, 264, 3532, 1 };

TFT_eSPI tft = TFT_eSPI();

// LVGL Stuff
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[ SCREEN_WIDTH * 10 ];
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p );
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data );


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

static uint8_t cardType = CARD_NONE;
static uint64_t cardSize = 0;

static String card_type(uint8_t cardType)
{
  String rtn { "UNKNOWN" };
  switch( cardType )
  {
    case CARD_NONE:
      rtn = { "CARD_NONE" };
      break;
    case CARD_MMC:
      rtn = { "MMC" };
      break;
    case CARD_SD:
      rtn = { "SDSC" };
      break;
    case CARD_SDHC:
      rtn = { "SDHC" };
      break;
    default:
      // default assigned at initialization
      ;
  } // switch( cardType )

  return rtn;
} // card_type()

// Temporary GUI stuff
lv_obj_t *tempLabel = nullptr;
lv_obj_t *setLabel = nullptr;
static void setTempLabel(String val)
{
  lv_label_set_text( tempLabel, val.c_str() );
} // setTempLabel

static void setSetLabel(String val)
{
  lv_label_set_text( setLabel, val.c_str() );
} // setSetLabel()

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("Hello World!");

 	//ESP32Encoder::useInternalWeakPullResistors=DOWN;
	// Enable the weak pull up resistors
	ESP32Encoder::useInternalWeakPullResistors=UP;
	encoder.attachHalfQuad(ENC1_Q1, ENC1_Q2);
  encoder.setCount(680); // 68.0 degrees, fixed point

  Serial.println("Encoder has been setup");

  pinMode(TFT_BACKLIGHT, OUTPUT);
  pinMode(TFT_BACKLIGHT, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  Serial.println("TFT pins have been setup");

  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);            // Clear screen

  tft.setFreeFont(&FreeSans18pt7b);        // Select the font
  tft.setTextColor(TFT_YELLOW, TFT_BLACK); // Yellow over black

  String LVGL_Arduino = String("LVGL Version: V") + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

  Serial.println( LVGL_Arduino );

  lv_init();
  tft.setTouch( TOUCH_CAL_DATA );

  lv_disp_draw_buf_init( &draw_buf, buf, NULL, SCREEN_WIDTH * 10 );

  // Initialize the display
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init( &disp_drv );
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register( &disp_drv );

  /*Initialize the (dummy) input device driver*/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init( &indev_drv );
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register( &indev_drv );

  Serial.println("TFT and LVGL has been set up");

  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();

  Serial.println("DHT has been setup");

  setup_button_handler();

  Serial.println("Button has been setup");

 // SD Card
  if ( SD.begin(SD_CS) ) 
  {
    cardType = SD.cardType();

    if ( cardType != CARD_NONE )
    {
      cardSize = SD.cardSize() / (1024 * 1024);
    }
    else
    {
      Serial.println("No SD card attached");
      return;
    }
  }
  else{
    Serial.println("Card Mount Failed");
  }

  Serial.println("SD has been setup");

  initialize_wdt(500, &wdt_ISR); // 500 msec

  // Initialize GUI
  // Create simple label
  tempLabel = lv_label_create( lv_scr_act() );
  setLabel = lv_label_create( lv_scr_act() );

  lv_obj_align( tempLabel, LV_ALIGN_CENTER, 0, -10 );
  lv_obj_align( setLabel, LV_ALIGN_CENTER, 0, 10 );

  setTempLabel("0.0");
  setSetLabel("000");
} // setup()

void loop() {
  static unsigned loop_cntr = 0;
  static const unsigned DELAY = 5;

  feed_wdt();


  // LED Blink Code
  if (loop_cntr % (1000/DELAY) == 0) // 1 second
  {
    static bool on = false;
    digitalWrite(LED_BUILTIN, on ? HIGH : LOW);   // turn the LED on (HIGH is the voltage level)
    on = !on;
  }

  if ( loop_cntr & (250/DELAY) )
  {
    // Get and report touch
    uint16_t x, y;

    tft.getTouchRaw(&x, &y);
    #if 0
    Serial.printf("x: %i     ", x);
    Serial.printf("y: %i     ", y);
    Serial.printf("z: %i \n", tft.getTouchRawZ());
    #endif
  }

  if ( loop_cntr % (10000/DELAY) == 0)
  {
    Serial.printf("SD Card Type: %s  Size: %lluMB\n", card_type(cardType), cardSize);
  }

  if (loop_cntr % (10000/DELAY) == 0)
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
      setTempLabel(String(f));
    }
  }

  if (loop_cntr % (100/DELAY) == 0)
  {

    static unsigned long oldPosition  = encoder.getCount();

    unsigned long newPosition = encoder.getCount();
    if (newPosition != oldPosition) {
      oldPosition = newPosition;
      Serial.println("Position: " + String(newPosition));
      setSetLabel(String(newPosition));
    }
  }

  if ((loop_cntr % (500/DELAY) == 0) && !digitalRead(ENC1_PB))
  {
    Serial.println("Pressed : Button Cnt: " + String(button_cnt));
  }

  if (loop_cntr % (10000/DELAY) == 0)
  {
    // Read light level
    auto light_level = analogRead(LIGHT_PIN);
    Serial.println("Light: " + String(light_level));
  }

  loop_cntr++;

  // Let the GUI do it's work
  lv_timer_handler();

  delay(DELAY);
} // loop()

/* LVGL: Display flush */
void my_disp_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
} // my_disp_flush()

/*Read the touchpad*/
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
} // my_touchpad_read()
