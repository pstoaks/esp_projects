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

    """Simple function for dimming lights

    Args:
        *args (one or more ints): Dim each of these lights
    """
    def send_command(self):
        self._pixel_object.write();
    
    def set_light(self, light_index, rgb_val):
        if light_index < 0:
            light_index = 0
        self._pixel_object[light_index] = rgb_val
        self.light_dict[light_index]["red"] = rgb_val[0]
        self.light_dict[light_index]["green"] = rgb_val[1]
        self.light_dict[light_index]["blue"] = rgb_val[2]
        
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
            red_value = int(color_vals["red"] * scale)
            green_value = int(color_vals["green"] * scale)
            blue_value = int(color_vals["blue"] * scale)
            if red_value < 0:
                red_value = 0
            if blue_value < 0:
                blue_value = 0
            if green_value < 0:
                green_value = 0
            self.set_light(light, (red_value, green_value, blue_value))
        self.send_command()

    
        
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
            
            
            
# controller = LightController(5, 64)
# controller.propagate_wave(250)

            
                
        
