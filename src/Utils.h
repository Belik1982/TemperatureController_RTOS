// Utils.h
#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include "Config.h"

// Инициализация буззера
void setupBuzzer();
// Воспроизведение звука с заданной частотой (Hz) и длительностью (ms)
void beep(int frequency, int duration);
// Установка громкости буззера (0-255)
void setBuzzerVolume(int volume);
// Короткий звуковой сигнал подтверждения
void confirmBeep();
// Звуковая сигнализация ошибки (несколько коротких сигналов)
void errorBeep();

#endif
