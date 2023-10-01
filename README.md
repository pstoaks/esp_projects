# esp_projects
A small collection of ESP8266 and ESP32 projects.

## micropython experiments
Just some example code. Part of it is a Pixel controller with a web interface. 
The following REST API is implemented along with a partial home page with some controls.

/leds?bright=10  *// Set brightness *
/leds?bright=20&series=winter  * // Set brightness and color series (color series can be added to the config file led_config.json) *
/leds?num_leds=256 * // Set the number of LEDs for a new string *
/config *// Returns the current configuration JSON. Edit and use WebREPL to upload the modified file to the device."
/exit  Exit server to that we can use REPL or WebREPL
