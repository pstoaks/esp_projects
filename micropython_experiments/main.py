# This file is executed on every boot (including wake-boot from deepsleep)
import random
import time
import machine
import onewire
import ds18x20
from light_controller import LightController

NUM_LEDS = 64 # Number of LEDs in the strip
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

def wifi_connect():
    import network
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    if not wlan.isconnected():
        print('connecting to network...')
        wlan.connect(SSID, PASSWORD)
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
        Note tha the variable replacement is very crude and there is no error handling.
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
        led_on = request.find('/?led=on')
        led_off = request.find('/?led=off')
        if led_on == 6:
            print('LED ON')
            on_off_state = True
            lc.rainbow(255, 80)
        if led_off == 6:
            print('LED OFF')
            on_off_state = False
            lc.turn_off_all()
            lc.send_command()
        response = web_page(on_off_state)
        send_http_response_header(conn)
        conn.sendall(response)
        conn.close()

########################### Demo Code Below ####################################

lc = LightController(NEO_DATA_PIN, NUM_LEDS)
temp = TempSensor(TEMP_DATA_PIN)
ana = AnalogReader(ANA_INP_PIN)

# Put your WiFi network credentials here.
SSID = "NOT_MY_REAL_SSID"
PASSWORD = "NOT_MY_REAL_WIFI_PASSWORD"

def test():
    wifi_connect()
    start_server()




