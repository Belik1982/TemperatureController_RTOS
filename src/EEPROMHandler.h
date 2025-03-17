// EEPROMHandler.h
#ifndef EEPROM_HANDLER_H
#define EEPROM_HANDLER_H

#include <Arduino.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "Config.h"
#include "BaseChannel.h"

// Инициализация EEPROM и мьютекса
void initEEPROM();
// Сохранение настроек (уставок) в EEPROM
void saveSettings();
// Загрузка настроек из EEPROM
void loadSettings();

#endif
