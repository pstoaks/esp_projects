# This file is executed on every boot (including wake-boot from deepsleep)
# https://docs.micropython.org/en/v1.15/library/uos.html
# uasyncio tutorial: https://github.com/peterhinch/micropython-async/blob/master/v3/docs/TUTORIAL.md
#
# TODO:
#    twinkle, march - This will require another COR, right?
#    Add file access handling for .HTML, .CSS, .JS, etc. Perhaps check for file if path is
#       not found? Or add a /file path that checks for file after. Would require some
#       changes to path parsing to make this work. /mypath/foo.css, only look for the 'mypath'
#       in ServerPaths and then pass the path on to the handler.
#    Learn ASYNCIO
#    Add brightness up/down buttons
#    Integrate with home controller
#
# SERVER PATHS
#  http://192.168.0.122/leds?bright=10  Set brightness
#  http://192.168.0.122/leds?bright=20&series=winter  Set brightness and color series
#  http://192.168.0.122/leds?num_leds=256 Set the number of LEDs for a new string
#  http://192.168.0.122/exit  Exit server to that we can use REPL or WebREPL
#  
import random
import time
import machine
import onewire
import ds18x20
import http_server as Server
import config as Config

# from light_controller import LightController
from pixel_controller import PixelController

NUM_LEDS = 86
NEO_DATA_PIN = 5  # Labeled G5 on board
TEMP_DATA_PIN = 15 # Labeled G15
ANA_INP_PIN = 34 # Labeled G34 on board
CONFIG_FILE_NAME = "led_config.json"


class TempSensor(object):
    
    def __init__(self, pin):
        ifc = onewire.OneWire(machine.Pin(pin))
        self._ds18x20 = ds18x20.DS18X20(ifc)
        self._sensors = self._ds18x20.scan()
        print('Found DS devices: ', self._sensors)

    def num_sensors(self):
        return len(self._sensors)
    
    def read_sensor(self, idx):
        """Pass the sensor index in idx (starting at zero).
              Note that it may be possible to send conver_temp() once
              and then read all sensors. This is just a convenient
              read function.
        """
        self._ds18x20.convert_temp()
        time.sleep_ms(750)
        return self._ds18x20.read_temp(self._sensors[idx])


class AnalogReader(object):
    def __init__(self, pin):
        self._pin = machine.ADC(machine.Pin(pin))
        self._pin.atten(machine.ADC.ATTN_11DB)

    def read(self):
        return self._pin.read()

def C_to_F(t):
    return (t * 9.0/5.0) + 32.0

def cnt_to_pct(cnt):
    return (cnt/4095) * 100.0

#################################### Network Setup #################################

def wifi_connect(ssid, passwd):
    import network
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print('connecting to network...')
        wlan.connect(ssid, passwd)
        while not wlan.isconnected():
            time.sleep(0.3)
    print('network config:', wlan.ifconfig())

#################################### HTTP Server #################################

def web_page(config):
    """ The file is opened and read each time so that it can be updated without
        restarting the program.
        Note that the variable replacement is very crude and there is no error handling.
        Better way to do it is to take a dictionary as an argument containing the variable
        values in the template document, then replace each key with mustaches around it
        (e.g. {{KEY_NAME}}) with the value from the dictionary.
        Also take the template file name as a parameter.
    """

    html = ""
    with open("home.html", "r") as fd:
        html = fd.read()

    tmplt = '<div class="series_button"><a href="leds?series={SERIES_NAME}"><button class="button {SELECT}">{SERIES_NAME}</button></a></div>'

    series_buttons = ""
    for (series,val) in config['color_series'].items():
        select = "SELECTED"if config['series'] == series else "NOT_SELECTED"
        series_buttons = series_buttons + tmplt.format(SERIES_NAME=series, SELECT=select)

    return html.format(
        BRIGHT=config['bright'],
        SERIES=config['series'],
        NUM_LEDS=config['num_leds'],
        SERIES_BUTTONS=series_buttons
        )


############### Server Methods ######################

def set_leds(query):
    global current_config
    tmp_config = current_config.copy() # Save a copy so that we can detect changes.
    
    for (fn, param) in query.items():
        if fn in SET_LEDS:
            SET_LEDS[fn](param)

    # apply any changes
    if current_config != tmp_config:
        print("Saving modified configuration.", current_config)
        apply_config(current_config)
        Config.save(current_config, CONFIG_FILE_NAME)
    else:
        print("Configuration unmodified by request.")

    return web_page(current_config) # Temporary


def set_brightness(brightness_pct):
    global current_config
    current_config['bright'] = int(brightness_pct)
    
def set_series(series_name):
    global current_config
    if series_name in current_config['color_series']:
        current_config['series'] = series_name

def set_led_count(count):
    global current_config
    if int(count) > 1:
        current_config['num_leds'] = int(count)

SET_LEDS = {
    "bright": set_brightness,
    "series": set_series,
    "num_leds": set_led_count
}


def get_config(dummy):
    global current_config
    return Config.web_return(current_config)

def home_page(query):
    return web_page(current_config)
            

def return_file(file_path):
    # Not really implemented yet. The file_path argument is really a query...
    await Server.send_file(file_path)


def exit_server(param):
    raise Exception("Terminated by HTTP Server.")
    return ""


SERVER_PATHS = {
    "/leds": set_leds,
    "/config": get_config,
    "/file": return_file,
    "/exit": exit_server,
    "/": home_page
}

def apply_config(config):
    series_name = current_config['series']
    if series_name in current_config['color_series']:
        lc.apply_series(current_config['color_series'][series_name])
    lc.set_brightness(current_config['bright']/100.0)


########################### Demo Code Below ####################################

# temp = TempSensor(TEMP_DATA_PIN)
# ana = AnalogReader(ANA_INP_PIN)


BROWN = (20.0, 98.0, 5.0) # HSV (H degrees, S%, V%)
RED = (0.0, 100.0, 50.0)
FALL_RED = (0.0, 100.0, 20.0)
ORANGE = (15.0, 100.0, 20.0)
YELLOW = (40.0, 100.0, 30.0)
YELLOW_GREEN = (86.0, 100.0, 50.0)
GREEN = (120.0, 100.0, 50.0)
DK_GREEN = (128.0, 100.0, 20.0)
WHITE = (0.0, 0.0, 80.0)
BLUE = (240.0, 100.0, 40.0)
LT_BLUE = (230.0, 80, 80.0)
PURPLE = (270.0, 80, 40.0)
PINK = (301.0, 99, 60.0)

COLOR_SERIES = {
    "fall": (BROWN, FALL_RED, ORANGE, YELLOW, ORANGE),
    "christmas1": (GREEN, GREEN, RED, RED, WHITE),
    "christmas2": (GREEN, GREEN, GREEN, RED, RED, RED),
    "multicolor": (GREEN, RED, BLUE, YELLOW, DK_GREEN, WHITE, ORANGE, PURPLE),
    "multicolor2": (GREEN, RED, BLUE),
    "winter": (WHITE, WHITE, BLUE, BLUE, LT_BLUE),
    "valentines": (PINK, WHITE, RED, WHITE),
    "valentines2": (PINK, PINK, PINK, PINK, PINK, PINK, PINK, PINK, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, RED, RED, RED, RED, RED, RED, RED, RED, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE),
    "spring": (YELLOW, YELLOW_GREEN, GREEN, DK_GREEN, YELLOW_GREEN),
    "spring2": (YELLOW, GREEN, PURPLE, DK_GREEN, PINK),
    "white": (WHITE, LT_BLUE)
}

DEFAULT_CONFIG = {
    "bright": 30,
    "series": "fall",
    "num_leds": 86,
    "color_series": COLOR_SERIES
}

current_config = DEFAULT_CONFIG;

def convert_series_lists_to_tuples(color_series):
    """ The JSON module turns tuples into JSON arrays which
        become python lists. We have to convert the series back
        to tuples.
    """
    for name, series in color_series.items():
        tuple_list = list()
        for color in series:
            tuple_list.append(tuple(color))
        color_series[name] = tuple(tuple_list)


# Load configuration file
if Server.exists(CONFIG_FILE_NAME):
    print("Loading config file '{}'.".format(CONFIG_FILE_NAME))
    current_config = Config.load(CONFIG_FILE_NAME)
    # Convert lists to tuples
    convert_series_lists_to_tuples(current_config['color_series'])
    # Update the series, if necessary
    if current_config['color_series'] != COLOR_SERIES:
        # Update the color series definitions on disk
        current_config['color_series'] = COLOR_SERIES
        Config.save(current_config, CONFIG_FILE_NAME)
else:
    print("Creating config file first time.")
    Config.save(current_config, CONFIG_FILE_NAME)

# The number of LEDs in the string is now in the configuration file,
# so we wait to initialize the LED controller until we have that info
lc = PixelController(NEO_DATA_PIN, current_config['num_leds'])
lc.set_brightness(0.0) # Initialize brightness to 0 to avoid very high initial current draw.
apply_config(current_config)


# Start WiFi and the server

wifi_connect(WIFI_SSID, WIFI_PASSWD)

import webrepl
webrepl.start()

# WebREPL daemon started on ws://192.168.0.119:8266

# Start the http server
Server.start(SERVER_PATHS)
