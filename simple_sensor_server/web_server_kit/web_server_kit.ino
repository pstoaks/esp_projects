// A simple sensor and control application that provides a number of
// sensors, two relays, and serves up their status via a web page.
// This is just a "Getting Started" application for an inexpensive
// set of components.  See the accompanying README for details.
//
// Requires the following Arduino libraries:
// DallasTemperature
// Wire
// Adafruit_ADS1X15

#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DallasTemperature.h>
#include <Wire.h>  // For the I2C interface
#include <Adafruit_ADS1015.h>

ADC_MODE(ADC_VCC) // Set the on-board ADC to read supply voltage.

// Set these to your desired credentials.
const char *ssid = "Sens_AP";  // SSID of the network the ESP8266 sets up.
const char *password = nullptr; // "password";  // password is unused to create an 'open' network.

// Setup some components
ESP8266WebServer  server(80);         // Start web server on port 80
OneWire           onewire(D1);        // The OneWire libary instance, attached to pin D1
DallasTemperature sensor(&onewire);   // The DallasTemperature instance.
Adafruit_ADS1115  ads;                // 16-bit ADS1115

// Setup some constants
static auto LED_PIN = D4;     // The onboard LED pin
static int LED_ON_T = 500;    // LED On time in milliseconds
static int LED_OFF_T = 500;   // LED Off time in milliseconds
static auto RELAY_1 = D2;     // Relay 1 output pin
static auto RELAY_2 = D7;     // Relay 2 output pin
static auto WATER_PIN = D0;   // The 'water the plant' LED.
static float SET_TEMP = 70.0; // Set temperature for thermostat behavior
static unsigned DUSK = 22000; // The light ADC counts value at which we consider it dark.
static unsigned DRY = 12000;  // The soil moisture ADC counts value at which we consider the plant
							  // dry. NOTE: Higher numbers for dryer soil.


// Forward function declarations.  These functions are defined after the loop() function.
static void do_flashing_led();
static void do_thermostat();
static void do_lamp_control();
static void do_soil_moisture_monitor();
static void init_sensor();
static float read_temp();
static float read_vcc();
static void init_ads();
static int16_t read_ads(unsigned input);
static char const* get_header();

// Some global state
struct State
{
   bool relay_1;  // Thermostat
   bool relay_2;  // Lamp
   bool is_dry;   // Moisture LED
}; // State
static State global_state;

// This function handles requests to http://192.168.4.1/
void handleRoot()
{
   Serial.println("Serving request to /.");
   // Report our status
   String body = String("<html>"+String(get_header())+"<body>")
	  + "<h1>Temperature: " + String(read_temp()) + "</h1>"
      + "<h1>Supply Voltage: " + String(read_vcc()) + " V</h1>"
	  + "<h1>Light Level: " + String(read_ads(0)) + "</h1>"
	  + "<h1>Soil moisture: " + String(read_ads(1)) + "</h1>"
	  + "<h1>Relay 1: " + String(global_state.relay_1 ? "ON" : "OFF") + "</h1>"
	  + "<h1>Relay 2: " + String(global_state.relay_2 ? "ON" : "OFF") + "</h2>"
	  + "<h1>Plant moisture: " + String(global_state.is_dry ? "DRY" : "OK") + "</h2>"
	  + "</body></html>";
   server.send(200, "text/html", body);
}

void setup()
{
	delay(1000);
	Serial.begin(115200);

	// Set some pin modes
	pinMode(LED_PIN, OUTPUT);  // On-board LED pin
	pinMode(RELAY_1, OUTPUT);  // Relay 1
	pinMode(RELAY_2, OUTPUT);  // Relay 2
	pinMode(WATER_PIN, OUTPUT);  // Water the plant LED
	digitalWrite(LED_PIN, HIGH);
	digitalWrite(RELAY_1, HIGH);
	digitalWrite(RELAY_2, HIGH);
	digitalWrite(WATER_PIN, HIGH);
	
	Serial.println();
	Serial.print("Configuring access point...");
	WiFi.softAP(ssid, password);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	server.on("/", handleRoot);
	server.begin();
	Serial.println("HTTP server started");

	// Initialize the I2C interface with our selected pins.
	// SDA = D6, SCL = D5, 
	Wire.begin(D6, D5);

	// Initialize the DS18B20 temperature sensor.
	init_sensor();

	// Initialize the ADS1015 A/D Converter.  This reads the light-level
	// and the soil moisture.
	init_ads();
} // setup()

void loop() {
   // This is where all of the heavy lifting occurs.
   server.handleClient();

   // Flash the LED
   do_flashing_led();

   // Check the temperature and control relay 1 based on the reading.
   do_thermostat();

   // Check the light and control relay 2
   do_lamp_control();

   // Check the soil moisture
   do_soil_moisture_monitor();

   // Take a 10msec rest
   delay(10);
}

void do_flashing_led()
{
   // Flash The LED
   static bool led_state = false;
   static unsigned state_change_time = millis() + LED_OFF_T;
   if ( millis() > state_change_time )
   {
	  // Change the state
	  led_state = !led_state;
	  if ( led_state )
	  {
		 // Turn the LED off
		 digitalWrite(LED_PIN, HIGH);
		 state_change_time = millis() + LED_OFF_T;
	  }
	  else
	  {
		 // Turn the LED on
		 digitalWrite(LED_PIN, LOW);
		 state_change_time = millis() + LED_ON_T;
	  }
   }
} // do_flashing_led()

void do_thermostat()
{
   auto temp = read_temp();
   float hysteresis = 0.01 * SET_TEMP; // Set hysteresis to 1% of the set temp
   if ( !global_state.relay_1 && (temp < ( SET_TEMP - hysteresis)) )
   {
	  // The relay is off and we've fallen far enough below the set temperature
	  // to turn it on.
	  global_state.relay_1 = true;
   }
   else if ( global_state.relay_1 && (temp > (SET_TEMP + hysteresis)) )
   {
	  // The relay is on and the temperature is risen far enough above teh set
	  // temperature to turn it off.
	  global_state.relay_1 = false;
   }
   
   if ( global_state.relay_1 )
	  digitalWrite(RELAY_1, LOW);
   else
	  digitalWrite(RELAY_1, HIGH);
} // do_thermostat()

void do_lamp_control()
{
   auto light = read_ads(0);
//   Serial.println("Light: " + String(light));
   unsigned hysteresis = static_cast<unsigned>(std::floor(0.01 * DUSK));

   if ( !global_state.relay_2 && (light < ( DUSK - hysteresis)) )
   {
	  // The relay is off and we've fallen far enough below the set DUSK level
	  // to turn it on.
	  global_state.relay_2 = true;
   }
   else if ( global_state.relay_2 && (light > (DUSK + hysteresis)) )
   {
	  // The relay is on and the temperature is risen far enough above the set
	  // DUSK level to turn it off.
	  global_state.relay_2 = false;
   }
   
   if ( global_state.relay_2 )
	  digitalWrite(RELAY_2, LOW);
   else
	  digitalWrite(RELAY_2, HIGH);
} // do_lamp_control()

void do_soil_moisture_monitor()
{
   // Note that this is just like the lamp control function above. The
   // right thing to do from a software development perspective is to
   // factor out the common algorithm and use it for both.  However,
   // I've left it like this to make it a little bit easier to understand
   // and to make it easier to create different behaviors for the two
   // controllers.
   // NOTE:  The moister the soil, the lower the moisture counts will be.
   // Higher numbers mean dryer soil, lower numbers mean moister soil.
   auto moisture = read_ads(1);
//   Serial.println("Moisture: " + String(moisture));
   unsigned hysteresis = static_cast<unsigned>(std::floor(0.01 * DRY));

   if ( !global_state.is_dry && (moisture > ( DRY - hysteresis)) )
   {
	  // The plant was moist and the moisture counts have risen far enough
	  // above the DRY threshold to consider the plant dry.
	  global_state.is_dry = true;
   }
   else if ( global_state.is_dry && (moisture < (DRY + hysteresis)) )
   {
	  // The plant was dry and the moisture counts have fallen far enough below
	  // the DRY threshold to set is_dry to false.
	  global_state.is_dry = false;
   }
   
   if ( global_state.is_dry )
	  digitalWrite(WATER_PIN, LOW);
   else
	  digitalWrite(WATER_PIN, HIGH);
} // do_soil_moisture_monitor()

////////////////////////////////////////////////////////////////////////////////
// DS18B20 Temperature Sensor
////////////////////////////////////////////////////////////////////////////////

// Initializes the DS18B20 sensor
void init_sensor()
{
   sensor.begin();

   sensor.setResolution(10U);
} // initsensor()

// Reads the temperature from the DS18B20 sensor.
float read_temp()
{
   static constexpr unsigned READ_TIMEOUT = { 500U }; // Set the read timeout so we don't get stuck
   bool dummy = false;
   float tempF = 0.0;
   float tempC = 0.0;
   unsigned end_time = millis() + READ_TIMEOUT;
	  
   do
   {
	  sensor.requestTemperatures();
	  delay(20);
	  tempC = sensor.getTempCByIndex(0);
	  tempF = sensor.getTempFByIndex(0);
   } while ( ((tempC == 85.0) || (tempC == DEVICE_DISCONNECTED_C)) && (millis() < end_time) );
	  
   return tempF;
} // read_temp()

////////////////////////////////////////////////////////////////////////////////
// On-board supply voltage sensor
////////////////////////////////////////////////////////////////////////////////

// Returns the average of 4 reads of the supply voltage
// Averaging just filters the input a bit.
float read_vcc()
{
   
   unsigned accum = ESP.getVcc();

   for (auto i=0; i < 4; i++)
   {
	  accum = (accum + ESP.getVcc()) / 2;
	  delay(5);
   }

   return accum / 1000.0;
} // read_vcc()

////////////////////////////////////////////////////////////////////////////////
// ADS1015
////////////////////////////////////////////////////////////////////////////////

void init_ads()
{
   // Note that in this kit, VDD is connected to 3V.  Analog input pins
   // should be connected to a signal between GND & VDD for single-ended
   // input.
   //
   // The ADC input range (or gain) can be changed via the following
   // functions, but be careful never to exceed VDD +0.3V max, or to
   // exceed the upper and lower limits if you adjust the input range!
   // Setting these values incorrectly may destroy your ADC!
   //                                                                ADS1015  ADS1115
   //                                                                -------  -------
   // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
   ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
   // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
   // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
   // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
   // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
   ads.begin();
} // init_ads()

int16_t read_ads(unsigned input)
{
   // Reads one input (AIN0-AIN3) are (0-3), respectively
   return ads.readADC_SingleEnded(input);
} // read_ads()

char const* get_header()
{
   // Returns the HTML header for the page.
   static char const* hdr = R"EOF(<head>
      <meta http-equiv="refresh" content="2; URL=http://192.168.4.1/">
   </head>)EOF";
   
   return hdr;
} // get_header()
