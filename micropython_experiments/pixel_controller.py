# This file is executed on every boot (including wake-boot from deepsleep)
import random
import time
import machine
import neopixel

"""Class for controlling WS2812B lights

Args:
    pin (int): Microcontroller pin to use
    count (int): Count of LEDs in string
"""
class PixelController(object):
    """ Internally, each LED color is an (H, S%, V%) where S and V are percentages and
        H is a value between 0 and 360. """
    
    def __init__(self, pin, count):
        self._pin = pin
        self.brightness = 0.50  # Brightness (0.0 to 1.0)
        self.light_count = count
        self.light_dict = [(0.0, 0.0, 0.0) for i in range(0, self.light_count)]

        self._pixel_object = neopixel.NeoPixel(machine.Pin(self._pin), self.light_count)


    def _pct_to_tff(self, pct):
        """ Turns a percentage into a value from 0 to 255. """
        return round(pct/100.0*255.0)


    def _deg_to_tff(self, deg):
        """ Turns degrees into a value from 0 to 255. """
        return round(deg/360.0*255.0)


    def hsv_to_rgb(self, hsv):
        """Returns an RGB tuple given an HSV (deg, %, %)"""

        H, S, V = hsv
        H = self._deg_to_tff(H)
        S = self._pct_to_tff(S)
        V = self._pct_to_tff(V)

        # Check if the color is Grayscale
        if S == 0:
            R = V
            G = V
            B = V
        else:
           # Make hue 0-5
           region = H // 43

           # Find remainder part, make it from 0-255
           remainder = (H - (region * 43)) * 6 

           # Calculate temp vars, doing integer multiplication
           P = (V * (255 - S)) >> 8
           Q = (V * (255 - ((S * remainder) >> 8))) >> 8
           T = (V * (255 - ((S * (255 - remainder)) >> 8))) >> 8

           # Assign temp vars based on color cone region
           if region == 0:
               R = V
               G = T
               B = P
           elif region == 1:
               R = Q
               G = V 
               B = P
           elif region == 2:
               R = P 
               G = V 
               B = T
           elif region == 3:
               R = P 
               G = Q 
               B = V
           elif region == 4:
               R = T 
               G = P 
               B = V
           else: 
               R = V 
               G = P 
               B = Q
               
        return (R, G, B)


    def _apply_brightness(self, hsv, brightness):
        """brightness: 0.0 to 1.0"""
        return (hsv[0], hsv[1], hsv[2]*brightness)


    def write(self):
        for light in range(0, self.light_count):
            self._pixel_object[light] = self.hsv_to_rgb(self._apply_brightness(self.light_dict[light], self.brightness))
        self._pixel_object.write()
    

    def set_light(self, light_index, hsv):
        if light_index < 0:
            light_index = 0
        self.light_dict[light_index] = hsv
        

    def get_light(self, light_index):
        if light_index < 0:
            light_index = 0
        return self.light_dict[light_index]


    def set_intensity(self, V, leds):
        """ Scales the intensity by scale """
        for light in leds:
            self.light_dict[light][2] = V


    def off(self):
        """Turn off all of the lights. The intensity of all of the LEDs is
           set to 0.0"""
        for light_index in range(0, self.light_count):
            self.light_dict[light_index][2] = 0.0
        self.write()


    def set_brightness(self, brightness):
        """Set's the brightness of the entire string to the given value.
           brighness: A value between 0.0 and 1.0."""
        self.brightness = brightness
        self.write()
        

    def apply_series(self, series):
        """ Repeats a series throughout the string.
            series: list of lists where each member of the series is [H, S, V]"""
        series_len = len(series)
        for idx in range(0, self.light_count):
            self.light_dict[idx] = series[idx%series_len]
        self.write()
        
