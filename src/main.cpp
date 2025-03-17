// TemperatureController_RTOS_autotune1503.ino
// Основной файл проекта. Все модули интегрированы с использованием FreeRTOS.
// Некоторые критические секции переработаны для минимизации блокировок.
// Модифицированы функции задач для использования неблокирующих задержек (vTaskDelay).
  
#include <Arduino.h>
#include <GyverMAX6675.h>
#include <EncButton.h>
#include <LiquidCrystal_PCF8574.h>
#include <EEPROM.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "Config.h"
#include "HeaterChannel.h"
#include "Globals.h"
#include "Emergency.h"
#include "Display.h"
#include "Utils.h"
#include "EEPROMHandler.h"
#include <driver/ledc.h>

// Глобальные объекты энкодеров
EncButton enc1(ENC1_DT, ENC1_CLK, ENC1_SW);
EncButton enc2(ENC2_DT, ENC2_CLK, ENC2_SW);
EncButton enc3(ENC3_DT, ENC3_CLK, ENC3_SW);

// Переменные для управления режимами и настройки уставок
unsigned long lastEncoderActionTime = 0;

// Функция обновления бегущей строки
void updateServiceMessage(const char* message) {
    if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(50))) {
        // /// MODIFIED: Обеспечиваем корректное завершение строки
        strncpy(baseServiceMsg, message, sizeof(baseServiceMsg) - 1);
        baseServiceMsg[sizeof(baseServiceMsg) - 1] = '\0';
        scrollIndex = 0;
        xSemaphoreGive(systemMutex);
    }
}

// Задача автотюнинга (заглушка). Исправлено: мьютекс освобождается до задержки.
void TaskAutotune(void *pvParameters) {
    while (1) {
        if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(50))) {
            if (systemMode == AUTOTUNE_MODE) {
                updateServiceMessage("***AUTOTUNE MODE***");
                xSemaphoreGive(systemMutex);  // /// MODIFIED: Выход из критической секции перед задержкой
                vTaskDelay(pdMS_TO_TICKS(1000));  // Неблокирующая задержка
                if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(50))) {
                    systemMode = STANDBY_MODE;
                    updateServiceMessage("***standby mode***");
                    xSemaphoreGive(systemMutex);
                }
            } else {
                xSemaphoreGive(systemMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Задача обновления энкодеров: обработка вращений, кликов, удержаний.
void TaskUpdateEncoders(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(20);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(30))) {
            EncButton* encoders[] = {&enc1, &enc2, &enc3};
            for (int i = 0; i < NUM_CHANNELS; i++) {
                EncButton* enc = encoders[i];
                enc->tick();
                
                // Обработка удержания
                if (enc->hold()) {
                    if (systemMode == STANDBY_MODE) {
                        if (i == 1) {
                            systemMode = SETTING_MODE;
                            updateServiceMessage("***SETTING MODE***");
                            confirmBeep();
                        } else if (i == 2) {
                            systemMode = CALIBRATION_MODE;
                            updateServiceMessage("***CALIBRATION MODE***");
                            confirmBeep();
                        } else if (i == 0 && enc3.hold()) { // Комбинация для автотюнинга
                            systemMode = AUTOTUNE_MODE;
                            updateServiceMessage("***AUTOTUNE MODE***");
                            confirmBeep();
                        }
                    } else if (systemMode == WORKING_MODE) {
                        systemMode = STANDBY_MODE;
                        updateServiceMessage("***standby mode***");
                        confirmBeep();
                    }
                }

                // Обработка клика
                if (enc->click()) {
                    if (systemMode == STANDBY_MODE) {
                        systemMode = WORKING_MODE;
                        updateServiceMessage("***working mode***");
                        confirmBeep();
                    } else if (systemMode == WORKING_MODE) {
                        if (!settingModeActive) {
                            settingModeActive = true;
                            activeChannel = i;
                            String msg = "***SET TEMP SP" + String(i + 1) + "***";
                            updateServiceMessage(msg.c_str());
                            lastEncoderActionTime = millis();
                            confirmBeep();
                        } else if (activeChannel == i) {
                            channels[i]->setSetpoint(channels[i]->getSetpoint());
                            settingModeActive = false;
                            activeChannel = -1;
                            updateServiceMessage("***working mode***");
                            confirmBeep();
                        }
                    }
                }

                // Обработка вращения энкодера
                if (enc->turn()) {
                    if (systemMode == STANDBY_MODE || (systemMode == WORKING_MODE && settingModeActive && activeChannel == i)) {
                        double delta = enc->dir() * 0.5;
                        double newSetpoint = channels[i]->getSetpoint() + delta;
                        channels[i]->setSetpoint(newSetpoint);
                        lastEncoderActionTime = millis();
                        confirmBeep();
                    }
                }
            }
            // Обработка таймаутов ввода
            if (systemMode == STANDBY_MODE && (millis() - lastEncoderActionTime > 2000)) {
                for (int i = 0; i < NUM_CHANNELS; i++) {
                    channels[i]->setSetpoint(channels[i]->getSetpoint());
                }
            } else if (settingModeActive && (millis() - lastEncoderActionTime > 5000)) {
                settingModeActive = false;
                if (activeChannel >= 0) {
                    channels[activeChannel]->setSetpoint(channels[activeChannel]->getSetpoint());
                }
                activeChannel = -1;
                updateServiceMessage("***working mode***");
                errorBeep();
            }
            xSemaphoreGive(systemMutex);
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Задача управления нагревателями: считывает температуру, обновляет PID и управляет выходом.
void TaskControlHeaters(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(100);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(50))) {
            for (int i = 0; i < NUM_CHANNELS; i++) {
                if (channels[i]) {
                    channels[i]->readAndUpdateTemperature();
                    if (systemMode == WORKING_MODE) {
                        channels[i]->updatePID();
                        channels[i]->controlHeater(channels[i]->getOutput());
                    } else {
                        channels[i]->controlHeater(0);
                    }
                }
            }
            xSemaphoreGive(systemMutex);
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// Задача обновления дисплея: обновляет данные по каналам и сервисное сообщение.
void TaskUpdateDisplay(void *pvParameters) {
    const TickType_t xFrequency = pdMS_TO_TICKS(250);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
            updateDisplay();
            xSemaphoreGive(displayMutex);
        } else {
            Serial.println("[DISPLAY] Не удалось захватить мьютекс для обновления!");
        }
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void setup() {
    Serial.begin(115200);
    initEEPROM();
    setupBuzzer();
    initDisplay();

    // Создаем мьютексы
    systemMutex = xSemaphoreCreateMutex();
    displayMutex = xSemaphoreCreateMutex();

    // Создаем датчики для системы (одновременно используются всеми каналами)
    GyverMAX6675<18, 17, 4>* sensor1 = new GyverMAX6675<18, 17, 4>();
    GyverMAX6675<18, 5, 16>* sensor2 = new GyverMAX6675<18, 5, 16>();
    GyverMAX6675<18, 19, 23>* sensor3 = new GyverMAX6675<18, 19, 23>();

    // Инициализация каналов нагревателей
    channels[0] = new HeaterChannel(sensor1, sensor2, sensor3, &enc1, HEATER1_PIN, 0, LEDC_TIMER_0, 0, DEFAULT_SETPOINT);
    channels[1] = new HeaterChannel(sensor1, sensor2, sensor3, &enc2, HEATER2_PIN, 1, LEDC_TIMER_1, 1, DEFAULT_SETPOINT);
    channels[2] = new HeaterChannel(sensor1, sensor2, sensor3, &enc3, HEATER3_PIN, 2, LEDC_TIMER_2, 2, DEFAULT_SETPOINT);

    // Загрузка настроек (уставок и калибровочных смещений) из EEPROM
    loadSettings();

    // Создание задач FreeRTOS
    xTaskCreate(TaskUpdateEncoders, "Encoders", 2048, NULL, 2, NULL);
    xTaskCreate(TaskControlHeaters, "Heaters", 2048, NULL, 1, NULL);
    xTaskCreate(TaskUpdateDisplay, "Display", 2048, NULL, 1, NULL);
    xTaskCreate(TaskAutotune, "Autotune", 2048, NULL, 1, NULL);

    if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(50))) {
        systemMode = STANDBY_MODE;
        updateServiceMessage("***standby mode***");
        xSemaphoreGive(systemMutex);
    }
}

void loop() {
    // Основной цикл пуст, так как управление осуществляется через FreeRTOS задачи.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
