// Config.h
#ifndef CONFIG_H
#define CONFIG_H

// Количество каналов системы
#define NUM_CHANNELS 3

// Размеры дисплея
#define DISPLAY_WIDTH 20
#define DISPLAY_HEIGHT 4

// Параметры PWM
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8
#define PWM_MAX_DUTY 255

// Диапазон уставок температуры
#define MIN_SETPOINT 0.0
#define MAX_SETPOINT 500.0
#define DEFAULT_SETPOINT 100.0

// Стартовые коэффициенты PID-регулятора
#define PID_KP 10.0
#define PID_KI 0.1
#define PID_KD 5.0

// Назначение пинов энкодеров (DT, CLK, SW)
#define ENC1_DT 2
#define ENC1_CLK 3
#define ENC1_SW 4
#define ENC2_DT 5
#define ENC2_CLK 6
#define ENC2_SW 7
#define ENC3_DT 8
#define ENC3_CLK 9
#define ENC3_SW 10

// Назначение пинов нагревателей
#define HEATER1_PIN 11
#define HEATER2_PIN 12
#define HEATER3_PIN 13

// Пин и канал для буззера
#define BUZZER_PIN 14
#define BUZZER_CHANNEL 3

#endif
