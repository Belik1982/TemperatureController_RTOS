// Display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <LiquidCrystal_PCF8574.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Globals.h"

// Функции для работы с дисплеем
void initDisplay();
void updateDisplay();
void updateChannelDisplay(uint8_t channel);
void blinkSetpoint(int channel);

// Внешнее объявление объекта дисплея
extern LiquidCrystal_PCF8574 lcd;

#endif
