# Full Bridge PWM Driver

## Overview

I need a full bridge PWM gate driver for a power switching project I'm working on.
***bit_banged_pwm*** is a pure software solution on the ESP8266.  Unfortunately,
the maximum usable PWM frequency is only between 2 and 3kHz, and I might need
faster than that.  Therefore I am also experimenting with the Motor Control PWM
features of the ESP32.  I'll post those results as soon as I have some.

