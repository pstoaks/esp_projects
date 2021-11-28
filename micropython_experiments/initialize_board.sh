#!/bin/bash

TTY=/dev/ttyUSB0

ampy --port ${TTY} --baud 115200 put boot.py
ampy --port ${TTY} --baud 115200 put main.py
ampy --port ${TTY} --baud 115200 put pixel_controller.py
ampy --port ${TTY} --baud 115200 put config.py
ampy --port ${TTY} --baud 115200 put http_server.py



