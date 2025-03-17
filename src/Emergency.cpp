// Emergency.cpp
// Реализация аварийного отключения системы.
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <cstring>
#include "Emergency.h"
#include "Globals.h"
#include "Utils.h"

void emergencyShutdown() {
    if (xSemaphoreTake(systemMutex, pdMS_TO_TICKS(1000))) {
        Serial.println("\n[EMERGENCY] Отключение!");
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (channels[i]) {
                channels[i]->controlHeater(0);
                channels[i]->emergencyStop();
            }
        }
        systemActive = false;
        systemMode = STANDBY_MODE;
        strncpy(baseServiceMsg, "АВАРИЯ", sizeof(baseServiceMsg));
        baseServiceMsg[sizeof(baseServiceMsg)-1] = '\0';
        for (int i = 0; i < 10; i++) {
            beep(2000, 100);
            vTaskDelay(pdMS_TO_TICKS(100)); // vTaskDelay вместо delay
        }
        updateServiceMessage("***standby mode***");
        Serial.println("[EMERGENCY] Аварийный режим завершён");
        xSemaphoreGive(systemMutex);
    } else {
        Serial.println("[EMERGENCY] Не удалось захватить мьютекс!");
    }
}
