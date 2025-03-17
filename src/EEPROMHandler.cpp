// EEPROMHandler.cpp
// Модуль для работы с EEPROM: сохранение и загрузка настроек, калибровочных смещений.
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "EEPROMHandler.h"
#include "Globals.h"

#define EEPROM_SETPOINT_ADDR (NUM_CHANNELS * sizeof(double))
#define EEPROM_SIZE (NUM_CHANNELS * sizeof(double) * 2)

static double prevSetpoints[NUM_CHANNELS] = {0}; // Предыдущие значения уставок
SemaphoreHandle_t eepromMutex = NULL;            // Мьютекс для доступа к EEPROM

void initEEPROM() {
    if (eepromMutex == NULL) {
        eepromMutex = xSemaphoreCreateMutex();
    }
    EEPROM.begin(EEPROM_SIZE);
}

void saveSettings() {
    if (!eepromMutex || !xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(100))) {
        Serial.println("[EEPROM] Не удалось захватить мьютекс для сохранения!");
        return;
    }
    bool needUpdate = false;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i]) {
            double currentSetpoint = channels[i]->getSetpoint();
            if (fabs(currentSetpoint - prevSetpoints[i]) > 0.01) {
                EEPROM.put(EEPROM_SETPOINT_ADDR + i * sizeof(double), currentSetpoint);
                prevSetpoints[i] = currentSetpoint;
                needUpdate = true;
            }
        }
    }
    if (needUpdate) {
        if (!EEPROM.commit()) {
            Serial.println("[EEPROM] Ошибка записи данных!");
        } else {
            Serial.println("[EEPROM] Настройки сохранены");
        }
    }
    xSemaphoreGive(eepromMutex);
}

void loadSettings() {
    if (!eepromMutex || !xSemaphoreTake(eepromMutex, pdMS_TO_TICKS(100))) {
        Serial.println("[EEPROM] Не удалось захватить мьютекс для загрузки!");
        return;
    }
    bool validData = true;
    double storedSetpoints[NUM_CHANNELS];
    for (int i = 0; i < NUM_CHANNELS; i++) {
        EEPROM.get(EEPROM_SETPOINT_ADDR + i * sizeof(double), storedSetpoints[i]);
        if (isnan(storedSetpoints[i]) || storedSetpoints[i] < MIN_SETPOINT || storedSetpoints[i] > MAX_SETPOINT) {
            validData = false;
            break;
        }
    }
    if (validData) {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i])
                continue;
            channels[i]->setSetpoint(storedSetpoints[i]);
            prevSetpoints[i] = storedSetpoints[i];
        }
        Serial.println("[EEPROM] Настройки загружены");
    } else {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i])
                continue;
            channels[i]->setSetpoint(DEFAULT_SETPOINT);
            prevSetpoints[i] = DEFAULT_SETPOINT;
            EEPROM.put(EEPROM_SETPOINT_ADDR + i * sizeof(double), DEFAULT_SETPOINT);
        }
        if (EEPROM.commit()) {
            Serial.println("[EEPROM] Установлены значения по умолчанию");
        } else {
            Serial.println("[EEPROM] Ошибка записи значений по умолчанию!");
        }
    }
    xSemaphoreGive(eepromMutex);
}
