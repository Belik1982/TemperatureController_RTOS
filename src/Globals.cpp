// Globals.cpp
// Определение глобальных переменных для проекта.
#include "Globals.h"

SystemMode systemMode = STANDBY_MODE;   // Текущий режим системы
SemaphoreHandle_t systemMutex = NULL;   // Мьютекс для глобальных ресурсов
SemaphoreHandle_t displayMutex = NULL;  // Мьютекс для дисплея
BaseChannel* channels[NUM_CHANNELS] = {nullptr};  // Массив указателей на каналы
bool systemActive = true;               // Флаг активности системы
bool settingModeActive = false;         // Флаг режима настройки
char baseServiceMsg[32] = "***standby mode***";  // Сервисное сообщение
int scrollIndex = 0;                    // Индекс бегущей строки
int activeChannel = -1;                 // Индекс активного канала (-1 - отсутствует)
