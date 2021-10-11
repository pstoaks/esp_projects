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
class LightController(object):
    BLUE = (0, 0, 255)
    RED = (255, 0, 0)
    GREEN = (0, 255, 0)
    WHITE = (255,255,255)
    OFF = (0, 0, 0)
    
    def __init__(self, pin, count):
        self._pin = pin
        self.light_count = count
        self._pixel_object = neopixel.NeoPixel(machine.Pin(self._pin), self.light_count)
        self.light_dict = {}
        for light in range(0, count):
            self.light_dict[light] = {"red": 0, "green": 0, "blue":0}

    def send_command(self):
        self._pixel_object.write()
    
    def set_light(self, light_index, rgb_val):
        if light_index < 0:
            light_index = 0
        self._pixel_object[light_index] = rgb_val
        self.light_dict[light_index]["red"] = rgb_val[0]
        self.light_dict[light_index]["green"] = rgb_val[1]
        self.light_dict[light_index]["blue"] = rgb_val[2]
        
    """Simple function for dimming lights

    Args:
        *args (one or more ints): Dim each of these lights
    """
    def dim_lights(self, dim_value, *args):
        for light in args:
            color_vals = self.light_dict[light]
            red_value = color_vals["red"] - dim_value
            green_value = color_vals["green"] - dim_value
            blue_value = color_vals["blue"] - dim_value
            if red_value < 0:
                red_value = 0
            if blue_value < 0:
                blue_value = 0
            if green_value < 0:
                green_value = 0
            self.set_light(light, (red_value, green_value, blue_value))
        self.send_command()

    def set_intensity(self, scale, *args):
        """ Scales the intensity by scale """
        for light in args:
            color_vals = self.light_dict[light]
            red_value = round(color_vals["red"] * scale)
            green_value = round(color_vals["green"] * scale)
            blue_value = round(color_vals["blue"] * scale)
            if red_value < 0:
                red_value = 0
            if blue_value < 0:
                blue_value = 0
            if green_value < 0:
                green_value = 0
            self.set_light(light, (red_value, green_value, blue_value))
        self.send_command()

    def get_light(self, idx):
        return self._pixel_object[idx]

    def get_light_hsv(self, idx):
        ''' Returns the given LED's settings in HSV '''

        RGB = self._pixel_object[idx]
        # Unpack the tuple for readability
        R, G, B = RGB

        # Compute the H value by finding the maximum of the RGB values
        RGB_Max = max(RGB)
        RGB_Min = min(RGB)

        # Compute the value
        V = RGB_Max
        if V == 0:
            H = S = 0
            return (H,S,V)

        # Compute the saturation value
        S = 255 * (RGB_Max - RGB_Min) // V

        if S == 0:
            H = 0
            return (H, S, V)

        # Compute the Hue
        if RGB_Max == R:
            H = 0 + 43*(G - B)//(RGB_Max - RGB_Min)
            if H < 0:
                H = H + 257
        elif RGB_Max == G:
            H = 85 + 43*(B - R)//(RGB_Max - RGB_Min)
        else: # RGB_MAX == B
            H = 171 + 43*(R - G)//(RGB_Max - RGB_Min)

        # print("RGB: ", RGB, " HSV: ", (H,S,V))
        return (H, S, V)


    def set_light_hsv(self, idx, HSV):
        ''' Converts an integer HSV tuple (value range from 0 to 255) to an RGB tuple 
            And sets the given LED to that value.
        '''

        # Unpack the HSV tuple for readability
        H, S, V = HSV

        # Check if the color is Grayscale
        if S == 0:
            R = V
            G = V
            B = V
            self.set_light(idx, (R, G, B))
            return

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

        # print("HSV: ", HSV, " RGB: ", (R, G, B))
        self.set_light(idx, (R, G, B))
        return


    def create_sine(self, *args):
        # might be difficult to do without taylor series
        return

    def turn_off_all(self):
        for light_index in range(0, self.light_count):
            self.set_light(light_index, self.OFF)
        

    def bouncer(self, val_in):
        last_light = self.light_count - 1
        if val_in <= last_light and val_in >= 0:
            val_out = val_in
        elif val_in >= 0:
            val_out = (last_light - val_in) + last_light
        else:
            val_out = -1
        return val_out

    """Propagates a blue and red wave down the led string

    Args:
        step_time (int): How long in milliseconds to wait before moving
                         on to the next light change
    """
    def propagate_wave(self, step_count, step_time):
        start_light = 0
        for _ in range(0, step_count):
            time.sleep(step_time/1000)
            self.turn_off_all()
            start_light += 1
            if start_light >= self.light_count*2:
                start_light = 0
            red_range = start_light - 3
            blue_range = start_light - 6
            
            for initial_index in range(blue_range, start_light):
                red_index = self.bouncer(initial_index + 3)
                blue_index = self.bouncer(initial_index)
                self.set_light(red_index, self.RED)
                self.set_light(blue_index, self.BLUE)

            self.send_command()

    def random_all(self):
        for light_index in range(0, self.light_count):
            red_value = random.randint(0, 255)
            green_value = random.randint(0, 255)
            blue_value = random.randint(0, 255)
            self.set_light(light_index, (red_value, green_value, blue_value))
        self.send_command()

    def random_color_set_brightness(self, brightness):
        def find_remaining(current):
            if current > 255:
                remaining = 255
            elif current < 0:
                remaining = 0
            else:
                remaining = current
            return remaining
            
        for light_index in range(0, self.light_count):
            remaining = brightness
            red_value = random.randint(0, find_remaining(remaining))
            remaining = remaining - red_value
            green_value = random.randint(0, find_remaining(remaining))
            remaining = remaining - green_value
            blue_value = random.randint(0, find_remaining(remaining))
            self.set_light(light_index, (red_value, green_value, blue_value))
        self.send_command()

    def step_random(self, step_count, step_time):
        for _ in range(0, step_count):
            time.sleep(step_time/1000)
            self.turn_off_all()
            self.random_all()

    def step_random_set_brightness(self, step_count, step_time, brightness):
        for _ in range(0, step_count):
            time.sleep(step_time/1000)
            self.turn_off_all()
            self.random_color_set_brightness(brightness)

    def rainbow(self, saturation, brightness):
        """ saturation and brightness are integers from 0 to 255. """
        MAX_HUE = 256
        increment = round(MAX_HUE/self.light_count)
        for i in range(0, self.light_count):
            self.set_light_hsv(i, (i*increment, saturation, brightness))
        self.send_command()

            
            
# controller = LightController(5, 64)
# controller.propagate_wave(250)

            
                
        
