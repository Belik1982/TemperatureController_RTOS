// HeaterChannel.cpp
// Реализация класса HeaterChannel для управления нагревателем, температурой и PID-регулятором.
#include <Arduino.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "HeaterChannel.h"
#include "Utils.h"

#define EEPROM_CALIB_OFFSET_ADDR 0
#define MAX_CALIB_OFFSET 50.0

// Конструктор: инициализирует датчики, энкодер и настраивает PWM через старый LEDC API.
HeaterChannel::HeaterChannel(GyverMAX6675<18, 17, 4>* sensor1,
                             GyverMAX6675<18, 5, 16>* sensor2,
                             GyverMAX6675<18, 19, 23>* sensor3,
                             EncButton* encoder,
                             uint8_t heaterPin,
                             uint8_t pwmChannel,
                             ledc_timer_t pwmTimer,
                             int channelIndex,
                             double defaultSP)
    : sensor1(sensor1), sensor2(sensor2), sensor3(sensor3), encoder(encoder),
      pid(PID_KP, PID_KI, PID_KD),
      heaterPin(heaterPin), pwmChannel(pwmChannel), pwmTimer(pwmTimer),
      channelIndex(channelIndex), setpoint(defaultSP), calibrationOffset(0.0), temperature(0.0)
{
    configurePWM();
    pid.setLimits(0, PWM_MAX_DUTY);

    EEPROM.get(EEPROM_CALIB_OFFSET_ADDR + channelIndex * sizeof(double), calibrationOffset);
    if (isnan(calibrationOffset) || fabs(calibrationOffset) > MAX_CALIB_OFFSET) {
        calibrationOffset = 0.0;
        EEPROM.put(EEPROM_CALIB_OFFSET_ADDR + channelIndex * sizeof(double), calibrationOffset);
        Serial.printf("[EEPROM] CH%d: Некорректное смещение, сброшено на 0\n", channelIndex + 1);
    }
}

// Настройка PWM с использованием старого API: ledc_timer_config и ledc_channel_config.
void HeaterChannel::configurePWM() {
    ledc_timer_config_t timerConf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(PWM_RESOLUTION),
        .timer_num = pwmTimer,
        .freq_hz = PWM_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timerConf);

    ledc_channel_config_t channelConf = {
        .gpio_num = heaterPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = static_cast<ledc_channel_t>(pwmChannel),
        .timer_sel = pwmTimer,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channelConf);
}

// Аварийная остановка: сброс интегральной составляющей удален, так как метода setIntegral нет
void HeaterChannel::emergencyStop() {
    // Если необходимо обнулить интегральную составляющую и поле доступно, можно использовать:
    // pid.integral = 0;  // при условии, что поле integral публичное
    controlHeater(0);
    errorBeep();
}

// Чтение и обновление температуры.
void HeaterChannel::readAndUpdateTemperature() {
    switch (channelIndex) {
        case 0: temperature = sensor1->readTemp(); break;
        case 1: temperature = sensor2->readTemp(); break;
        case 2: temperature = sensor3->readTemp(); break;
        default: temperature = NAN; break;
    }
    if (temperature < -100 || isnan(temperature)) {
        Serial.printf("[ERROR] CH%d: Неисправность датчика\n", channelIndex + 1);
        emergencyStop();
    }
}

// Обновление PID-регулятора.
void HeaterChannel::updatePID() {
    pid.setpoint = setpoint;
    pid.input = getTemperature();
    pid.getResult();
    static bool wasReached[NUM_CHANNELS] = {false};
    if (fabs(getTemperature() - setpoint) < 0.5 && !wasReached[channelIndex]) {
        confirmBeep();
        wasReached[channelIndex] = true;
    }
    else if (fabs(getTemperature() - setpoint) >= 0.5) {
        wasReached[channelIndex] = false;
    }
}

// Управление нагревателем с использованием старых функций: ledcWrite.
void HeaterChannel::controlHeater(int value) {
    ledcWrite(static_cast<ledc_channel_t>(pwmChannel), value);
}

// Обработка событий энкодера для изменения уставки.
void HeaterChannel::processEncoder(unsigned long currentMillis, bool &changedFlag) {
    encoder->tick();
    if (encoder->turn()) {
        double delta = encoder->dir() * 0.5;
        setpoint += delta;
        setpoint = constrain(setpoint, MIN_SETPOINT, MAX_SETPOINT);
        changedFlag = true;
    }
}
