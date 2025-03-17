// Utils.cpp
// Реализация утилитарных функций для управления буззером (звуковая сигнализация).
#include <Arduino.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Utils.h"

static int buzzerVolume = 128; // Громкость по умолчанию (0–255)

// Инициализация буззера с использованием старого LEDC API: ledcSetup и ledcAttachPin.
void setupBuzzer() {
    ledcSetup(BUZZER_CHANNEL, 1000, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
}

// Воспроизведение звука с заданной частотой и длительностью.
void beep(int frequency, int duration) {
    ledcWriteTone(BUZZER_CHANNEL, frequency);
    ledcWrite(BUZZER_CHANNEL, buzzerVolume);
    vTaskDelay(pdMS_TO_TICKS(duration));
    ledcWriteTone(BUZZER_CHANNEL, 0);
}

// Установка громкости буззера.
void setBuzzerVolume(int volume) {
    buzzerVolume = constrain(volume, 0, 255);
    Serial.printf("[BUZZER] Установлена громкость: %d\n", buzzerVolume);
}

// Короткий звуковой сигнал подтверждения.
void confirmBeep() {
    beep(1000, 100);
}

// Звуковая сигнализация ошибки (три коротких сигнала).
void errorBeep() {
    for (int i = 0; i < 3; i++) {
        beep(2000, 100);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
