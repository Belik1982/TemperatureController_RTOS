// AutoTune.cpp
// Реализация процедуры автотюнинга PID-регулятора по методу Зиглера-Николса.
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "AutoTune.h"
#include "Globals.h"
#include "Config.h"
#include "Utils.h"

void autoTunePID() {
    const unsigned long tuningDuration = 30000; // 30 секунд
    unsigned long startTime = millis();
    unsigned long lastPeakTime = startTime;
    double peakTemp = channels[0]->getTemperature();
    int oscillationCount = 0;
    double output = 50.0; // Начальное значение мощности

    // Установка начального значения мощности для первого канала
    channels[0]->controlHeater(static_cast<int>(output * PWM_MAX_DUTY / 100));

    while (millis() - startTime < tuningDuration) {
        double currentTemp = channels[0]->getTemperature();
        if (currentTemp > peakTemp + 1.0) {
            unsigned long now = millis();
            double currentTu = (now - lastPeakTime) / 1000.0; // Период в секундах
            lastPeakTime = now;
            peakTemp = currentTemp;
            oscillationCount++;
            Serial.printf("[AUTOTUNE] Пик зафиксирован. Tu = %.2f s, count = %d\n", currentTu, oscillationCount);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    double Tu = 5.0;
    if (oscillationCount > 0) {
        Tu = (millis() - startTime) / 1000.0 / oscillationCount;
    }
    double Ku = output;
    double newKp = 0.6 * Ku;
    double newKi = 2.0 * Ku / Tu;
    double newKd = Ku * Tu / 8.0;
    for (int i = 0; i < NUM_CHANNELS; i++) {
        if (channels[i]) {
            channels[i]->getPID().Kp = newKp;
            channels[i]->getPID().Ki = newKi;
            channels[i]->getPID().Kd = newKd;
        }
    }
    Serial.printf("[AUTOTUNE] Завершено. Kp=%.2f, Ki=%.2f, Kd=%.2f\n", newKp, newKi, newKd);
}
