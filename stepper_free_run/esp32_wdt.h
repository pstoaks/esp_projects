#pragma once

// Using Arduino ESP32 WatchdogTimer example sketch as basis.
// Uses hardware timer 0 to implement a watchdog timer.

// For more on the hardware timers, see:
// https://github.com/espressif/esp-idf/blob/595688a32ad653d8e6cb1c7682b813f96125853e/examples/peripherals/timer_group/main/timer_group_example_main.c
// API Docs:  http://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/timer.html


#include "esp_system.h"

hw_timer_t *wdt_timer = NULL;
static const unsigned TIMER_DIV = 80U;
static const bool COUNT_UP = true;
static const bool RISING_EDGE = true;
static const bool AUTO_RELOAD = true;
static const bool NO_AUTO_RELOAD = false;
typedef void (*TimerISR)(void );

void IRAM_ATTR reset_module()
{
   ets_printf("reboot\n");
   esp_restart();
} // reset_module()

void initialize_wdt(uint32_t timeout_ms, TimerISR wdt_isr)
{
   wdt_timer = timerBegin(0, TIMER_DIV, COUNT_UP);          //timer 0, div 80
   timerAttachInterrupt(wdt_timer, wdt_isr, RISING_EDGE);   //attach callback
   timerAlarmWrite(wdt_timer, timeout_ms * 1000, NO_AUTO_RELOAD); //set time in us
   timerAlarmEnable(wdt_timer);   //enable interrupt
} // initialize_wdt()

void feed_wdt()
{
   // Write a zero to the timer counter
   timerWrite(wdt_timer, 0);
} // feed_wdt()
