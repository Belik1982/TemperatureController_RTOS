#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>    // Подключаем первым для правильной инициализации
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "Config.h"
#include "BaseChannel.h"

// Перечисление режимов работы системы
enum SystemMode {
    STANDBY_MODE,    // Ожидание
    WORKING_MODE,    // Работа
    SETTING_MODE,    // Режим настройки уставок
    CALIBRATION_MODE,// Калибровка
    AUTOTUNE_MODE,   // Автоматическая настройка PID
    MANUAL_MODE      // Ручное управление
};

extern SystemMode systemMode;
extern SemaphoreHandle_t systemMutex;
extern SemaphoreHandle_t displayMutex;
extern BaseChannel* channels[NUM_CHANNELS];
extern bool systemActive;
extern bool settingModeActive;
extern char baseServiceMsg[32];
extern int scrollIndex;
extern int activeChannel;

// Функция обновления сервисного сообщения
void updateServiceMessage(const char* message);

#endif
