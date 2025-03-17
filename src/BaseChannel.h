// BaseChannel.h
#ifndef BASE_CHANNEL_H
#define BASE_CHANNEL_H

#include <GyverPID.h>

// Абстрактный базовый класс для каналов управления нагревателями.
// Все конкретные реализации (например, HeaterChannel) должны реализовывать данные методы.
class BaseChannel {
public:
    virtual ~BaseChannel() = default;
    virtual void emergencyStop() = 0;
    virtual void readAndUpdateTemperature() = 0;
    virtual void updatePID() = 0;
    virtual void controlHeater(int value) = 0;
    virtual void processEncoder(unsigned long currentMillis, bool &changedFlag) = 0;
    virtual void updateDisplay() = 0;
    
    virtual GyverPID& getPID() = 0;
    virtual double getSetpoint() const = 0;
    virtual void setSetpoint(double sp) = 0;
    virtual double getTemperature() const = 0;
    virtual int getOutput() = 0;
};

#endif
