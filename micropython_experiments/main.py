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

lc = LightController(NEO_DATA_PIN, NUM_LEDS)
temp = TempSensor(TEMP_DATA_PIN)
ana = AnalogReader(ANA_INP_PIN)


