// HeaterChannel.h
#ifndef HEATER_CHANNEL_H
#define HEATER_CHANNEL_H

#include <GyverMAX6675.h>
#include <EncButton.h>
#include <GyverPID.h>
#include <EEPROM.h>
#include "BaseChannel.h"
#include "Config.h"
#include <driver/ledc.h>

// Класс HeaterChannel, реализующий управление нагревателем посредством термопары, энкодера, PWM и PID.
class HeaterChannel : public BaseChannel {
public:
    // Конструктор: принимает указатели на датчики, энкодер, пин нагревателя, PWM-параметры, индекс канала и начальную уставку.
    HeaterChannel(GyverMAX6675<18, 17, 4>* sensor1,  // Канал 1
                  GyverMAX6675<18, 5, 16>* sensor2,  // Канал 2
                  GyverMAX6675<18, 19, 23>* sensor3, // Канал 3
                  EncButton* encoder, 
                  uint8_t heaterPin, 
                  uint8_t pwmChannel,     // /// MODIFIED: Этот параметр теперь не используется, но остается для совместимости
                  ledc_timer_t pwmTimer,    // также не используется в новой LEDC API
                  int channelIndex, 
                  double defaultSP);
    ~HeaterChannel() override = default;
    
    void emergencyStop() override;
    void readAndUpdateTemperature() override;
    void updatePID() override;
    void controlHeater(int value) override;
    void processEncoder(unsigned long currentMillis, bool &changedFlag) override;
    void updateDisplay() override {}  // Не используется в данном классе

    GyverPID& getPID() override { return pid; }
    double getSetpoint() const override { return setpoint; }
    void setSetpoint(double sp) override { setpoint = constrain(sp, MIN_SETPOINT, MAX_SETPOINT); }
    // Возвращает фактическую температуру с учетом калибровочного смещения.
    double getTemperature() const override { return temperature + calibrationOffset; }
    int getOutput() override { return static_cast<int>(pid.getResult()); }

private:
    // Датчики температуры
    GyverMAX6675<18, 17, 4>* sensor1;
    GyverMAX6675<18, 5, 16>* sensor2;
    GyverMAX6675<18, 19, 23>* sensor3;
    EncButton* encoder; // Указатель на энкодер
    GyverPID pid;       // PID-регулятор

    // Аппаратные параметры
    uint8_t heaterPin;  // Пин, к которому подключен нагреватель
    uint8_t pwmChannel; // Не используется в новой реализации
    ledc_timer_t pwmTimer; // Не используется в новой реализации
    int channelIndex;   // Индекс канала (0,1,2)
    double setpoint;    // Заданная уставка температуры
    double temperature; // Измеренная температура
    double calibrationOffset; // Калибровочное смещение (считывается из EEPROM)

    // Метод для настройки LEDC нового API.
    void configurePWM();
};

#endif
