// Display.cpp
// Модуль отображения информации на 20x4 LCD-дисплее для устройства.
// Первые три строки (0–2) отображают информацию по каналам в формате:
// "Tn:XXXC°SP:XXXC°PPP%"
// Четвёртая строка используется для отображения режима работы системы.
#include "Display.h"
#include "Globals.h"
#include <Arduino.h>

// Инициализация объекта дисплея с I2C-адресом 0x27
LiquidCrystal_PCF8574 lcd(0x27);

// Инициализация дисплея: очищаем экраны
void initDisplay() {
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
        lcd.begin(DISPLAY_WIDTH, DISPLAY_HEIGHT);
        // Заполняем все строки 20 пробелами для очистки
        for (uint8_t row = 0; row < DISPLAY_HEIGHT; row++) {
            lcd.setCursor(0, row);
            lcd.print("                    ");
        }
        xSemaphoreGive(displayMutex);
    } else {
        Serial.println("[DISPLAY] Не удалось захватить мьютекс для инициализации!");
    }
}

// Обновление строки для одного канала по формату: "Tn:XXXC°SP:XXXC°PPP%"
// Пример: "T1:025C°SP:100C°100%"
void updateChannelDisplay(uint8_t channel) {
    // Получаем текущую температуру и уставку как целые значения
    int currentTemp = static_cast<int>(channels[channel]->getTemperature());
    int currentSet = static_cast<int>(channels[channel]->getSetpoint());
    // Расчет мощности: процент от максимальной мощности
    int out = channels[channel]->getOutput();
    int percent = (out * 100) / PWM_MAX_DUTY;
    
    char buffer[21]; // 20 символов + нулевой терминатор
    // Формат: "T%d:%03dC°SP:%03dC°%3d%%"
    // %03d – выводит число в 3 символа с ведущими нулями.
    // %3d – выводит число с правым выравниванием в 3 символа.
    snprintf(buffer, sizeof(buffer), "T%d:%03dC°SP:%03dC°%3d%%", channel + 1, currentTemp, currentSet, percent);
    
    // Выводим строку для данного канала на дисплее
    lcd.setCursor(0, channel);
    lcd.print(buffer);
}

// Формирование строки для отображения режима работы.
// Формат: "****<MODE> MODE<extraStars>"
// Общая длина строки – 20 символов.
String formatModeString(SystemMode mode) {
    char modeStr[16];
    switch (mode) {
       case STANDBY_MODE:     strcpy(modeStr, "STANDBY");     break;
       case WORKING_MODE:     strcpy(modeStr, "WORKING");     break;
       case SETTING_MODE:     strcpy(modeStr, "SETTING");     break;
       case CALIBRATION_MODE: strcpy(modeStr, "CALIBRATION"); break;
       case AUTOTUNE_MODE:    strcpy(modeStr, "AUTOTUNE");    break;
       case MANUAL_MODE:      strcpy(modeStr, "MANUAL");      break;
       default:               strcpy(modeStr, "UNKNOWN");     break;
    }
    int lenMode = strlen(modeStr);
    // Формат: 4 символа звезд + длина modeStr + 5 символов " MODE" = 4 + lenMode + 5 = (lenMode + 9)
    int extraStars = 20 - (lenMode + 9);
    if (extraStars < 0) extraStars = 0;
    
    char extra[12];
    for (int i = 0; i < extraStars; i++) {
        extra[i] = '*';
    }
    extra[extraStars] = '\0';
    
    char fullBuffer[21];
    snprintf(fullBuffer, sizeof(fullBuffer), "****%s MODE%s", modeStr, extra);
    return String(fullBuffer);
}

// Обновление дисплея: обновляются первые три строки для каналов и четвёртая строка для режима.
void updateDisplay() {
    static int lastTemps[NUM_CHANNELS] = {0};
    static int lastSetpoints[NUM_CHANNELS] = {0};
    
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
        // Обновляем данные для каждого канала
        for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i]) continue;
            int currentTemp = static_cast<int>(channels[i]->getTemperature());
            int currentSet = static_cast<int>(channels[i]->getSetpoint());
            if (abs(currentTemp - lastTemps[i]) >= 1 || currentSet != lastSetpoints[i]) {
                updateChannelDisplay(i);
                lastTemps[i] = currentTemp;
                lastSetpoints[i] = currentSet;
            }
        }
        // Обновляем режим работы на 4-й строке
        String modeStr = formatModeString(systemMode);
        char modeLine[DISPLAY_WIDTH + 1];
        strncpy(modeLine, modeStr.c_str(), DISPLAY_WIDTH);
        modeLine[DISPLAY_WIDTH] = '\0';
        lcd.setCursor(0, DISPLAY_HEIGHT - 1);
        lcd.print(modeLine);
        xSemaphoreGive(displayMutex);
    } else {
        Serial.println("[DISPLAY] Не удалось захватить мьютекс для обновления!");
    }
}

// Функция мигания для режима настройки для активного канала.
// Если требуется показывать мигание вместо постоянного вывода текущих значений.
void blinkSetpoint(int channel) {
    static unsigned long lastBlinkTime = 0;
    static bool blinkState = false;
    unsigned long currentMillis = millis();
    if (currentMillis - lastBlinkTime > 500) {
        blinkState = !blinkState;
        lastBlinkTime = currentMillis;
    }
    if (blinkState) {
        char buffer[21];
        // Выводим заполнители вместо значения, например:
        snprintf(buffer, sizeof(buffer), "T%d:___C SP:___C ___%%", channel + 1);
        lcd.setCursor(0, channel);
        lcd.print(buffer);
    } else {
        updateChannelDisplay(channel);
    }
}
