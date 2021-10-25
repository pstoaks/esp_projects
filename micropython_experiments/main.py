# This file is executed on every boot (including wake-boot from deepsleep)
import random
import time
import machine
import onewire
import ds18x20

# from light_controller import LightController
from pixel_controller import PixelController

# NUM_LEDS = 472 # Number of LEDs in the strip
NUM_LEDS = 86
NEO_DATA_PIN = 5  # Labeled G5 on board
TEMP_DATA_PIN = 15 # Labeled G15
ANA_INP_PIN = 34 # Labeled G34 on board


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
            pass
    print('network config:', wlan.ifconfig())

#################################### HTTP Server #################################

try:
  import usocket as socket
except:
  import socket

def web_page(on_off_state):
    """ The file is opened and read each time so that it can be updated without
        restarting the program.
        Note that the variable replacement is very crude and there is no error handling.
        Better way to do it is to take a dictionary as an argument containing the variable
        values in the template document, then replace each key with mustaches around it
        (e.g. {{KEY_NAME}}) with the value from the dictionary.
        Also take the template file name as a parameter.
    """
    if on_off_state:
        gpio_state = "ON"
    else:
        gpio_state = "OFF"

    html = ""
    with open("home.html", "r") as fd:
        html = fd.read()

    return html.replace("{{GPIO_STATE}}", gpio_state)


def send_http_response_header(conn):
        conn.send('HTTP/1.1 200 OK\n')
        conn.send('Content-Type: text/html\n')
        conn.send('Connection: close\n\n')


def start_server():
    """ Never returns. If there is something you need to do in a loop, then
        put it in the while loop for now. This should be re-factored
        for any real work. Essentially, there wold be a dictionary of request
        handlers with the correct handler getting called depending on the
        request.

        Requests:
           /?bright=120   Set brightness to 120%
           /?series=fall  Set series to fall
    """
    on_off_state = False
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('', 80))  # Port 80 is the default HTTP server port.
    s.listen(5)       # Maximum of 5 queued connections

    on_off_state = False
    while True:
        conn, addr = s.accept()
        print('Got a connection from %s' % str(addr))
        request = conn.recv(1024)
        request = str(request)
        print('Content = %s' % request)
        response = ""
        if len(request) > 1:
            req = get_req_from_response(request)
            path = req['path']
            if path in SERVER_PATHS:
                response = SERVER_PATHS[path](req['query'])
            else:
                response = SERVER_PATHS['/']("")
        send_http_response_header(conn)
        conn.sendall(response)
        conn.close()
        time.sleep(0.3)


def get_req_from_response(response):
    req = response.split("\r\n")[0].split()[1].split("?")
    path = req[0]
    query = parse_query(req[1])
    return {"path": path, "query": query}
    
    
def parse_query(query):
    rtn = {}
    if query:
        sq = query.split('&')
        for x in sq:
            y = x.split('=')
            if len(y) == 2:
                rtn[y[0]] = y[1]
    return rtn


############### Server Methods ######################
def set_leds(query):
    for (fn, param) in query.items():
        if fn in SET_LEDS:
            SET_LEDS[fn](param)
    return web_page(1) # Temporary


def home_page(query):
    return web_page(1) # Temporary
            

def set_brightness(brightness_pct):
    lc.set_brightness(int(brightness_pct)/100.0)
    

def set_series(series_name):
    if series_name in COLOR_SERIES:
        lc.apply_series(COLOR_SERIES[series_name])
        

########################### Demo Code Below ####################################

lc = PixelController(NEO_DATA_PIN, NUM_LEDS)
temp = TempSensor(TEMP_DATA_PIN)
ana = AnalogReader(ANA_INP_PIN)


BROWN = (20.0, 98.0, 5.0) # HSV (H degrees, S%, V%)
RED = (0.0, 100.0, 50.0)
FALL_RED = (0.0, 100.0, 20.0)
ORANGE = (15.0, 100.0, 20.0)
YELLOW = (40.0, 100.0, 30.0)
YELLOW_GREEN = (86.0, 100.0, 50.0)
GREEN = (120.0, 100.0, 50.0)
DK_GREEN = (128.0, 100.0, 20.0)
WHITE = (0.0, 0.0, 80.0)
BLUE = (250.0, 100.0, 60.0)
LT_BLUE = (250.0, 80, 80.0)

COLOR_SERIES = {
    "fall": (BROWN, FALL_RED, ORANGE, YELLOW, ORANGE),
    "christmas": (GREEN, GREEN, RED, RED, WHITE),
    "winter": (WHITE, WHITE, BLUE, BLUE, LT_BLUE),
    "spring": (YELLOW, YELLOW_GREEN, GREEN, DK_GREEN, YELLOW_GREEN),
    "white": (WHITE, LT_BLUE)
}

SERVER_PATHS = {
    "/leds": set_leds,
    "/": home_page
}

SET_LEDS = {
    "bright": set_brightness,
    "series": set_series
}

# Set defaults
set_series("fall")
set_brightness("50")

# Start the server
# start_server()


