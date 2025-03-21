/*
    GyverPID - библиотека PID регулятора для Arduino
    Документация: https://alexgyver.ru/gyverpid/
    GitHub: https://github.com/GyverLibs/GyverPID
    Возможности:
    - Время одного расчёта около 70 мкс
    - Режим работы по величине или по её изменению (для интегрирующих процессов)
    - Возвращает результат по встроенному таймеру или в ручном режиме
    - Встроенные калибровщики коэффициентов
    - Режим работы по ошибке и по ошибке измерения
    - Встроенные оптимизаторы интегральной суммы

    AlexGyver, alex@alexgyver.ru
    https://alexgyver.ru/
    MIT License

    Версии:
    v1.1 - убраны дефайны
    v1.2 - возвращены дефайны
    v1.3 - вычисления ускорены, библиотека облегчена
    v2.0 - логика работы чуть переосмыслена, код улучшен, упрощён и облегчён
    v2.1 - integral вынесен в public
    v2.2 - оптимизация вычислений
    v2.3 - добавлен режим PID_INTEGRAL_WINDOW
    v2.4 - реализация внесена в класс
    v3.0
        - Добавлен режим оптимизации интегральной составляющей (см. доку)
        - Добавлены автоматические калибровщики коэффициентов (см. примеры и доку)
    v3.1 - исправлен режиме ON_RATE, добавлено автоограничение инт. суммы
    v3.2 - чуть оптимизации, добавлена getResultNow
    v3.3 - в тюнерах можно передать другой обработчик класса Stream для отладки
*/

#ifndef _GyverPID_h
#define _GyverPID_h
#include <Arduino.h>

#if defined(PID_INTEGER)  // расчёты с целыми числами
typedef int16_t pidtype;
#else  // расчёты с float числами
typedef float pidtype;
#endif

#define NORMAL 0
#define REVERSE 1
#define ON_ERROR 0
#define ON_RATE 1

class GyverPID {
   public:
    // ==== pidtype это float или int, в зависимости от выбранного (см. пример integer_calc) ====
    GyverPID() {}
	void reset() {
        integral = 0;
        prevInput = 0;
    }
    // kp, ki, kd, dt
    GyverPID(float new_kp, float new_ki, float new_kd, uint32_t new_dt = 100) {
        setDt(new_dt);
        Kp = new_kp;
        Ki = new_ki;
        Kd = new_kd;
    }

    // направление регулирования: NORMAL (0) или REVERSE (1)
    void setDirection(bool direction) {
        _direction = direction;
    }

    // режим: работа по входной ошибке ON_ERROR (0) или по изменению ON_RATE (1)
    void setMode(bool mode) {
        _mode = mode;
    }

    // лимит выходной величины (например для ШИМ ставим 0-255)
    void setLimits(int16_t min_output, int16_t max_output) {
        _minOut = min_output;
        _maxOut = max_output;
    }

    // установка времени дискретизации (для getResultTimer)
    void setDt(uint32_t new_dt) {
        _dt_s = new_dt / 1000.0f;
        _dt = new_dt;
    }

    pidtype setpoint = 0;  // заданная величина, которую должен поддерживать регулятор
    pidtype input = 0;     // сигнал с датчика (например температура, которую мы регулируем)
    pidtype output = 0;    // выход с регулятора на управляющее устройство (например величина ШИМ или угол поворота серво)
    float Kp = 0.0;        // коэффициент P
    float Ki = 0.0;        // коэффициент I
    float Kd = 0.0;        // коэффициент D
    float integral = 0.0;  // интегральная сумма

    // возвращает новое значение при вызове (если используем свой таймер с периодом dt!)
    pidtype getResult() {
        pidtype error = setpoint - input;         // ошибка регулирования
        pidtype delta_input = prevInput - input;  // изменение входного сигнала за dt
        prevInput = input;                        // запомнили предыдущее
        if (_direction) {                         // смена направления
            error = -error;
            delta_input = -delta_input;
        }
        output = _mode ? 0 : (error * Kp);   // пропорциональая составляющая
        output += delta_input * Kd / _dt_s;  // дифференциальная составляющая

#if (PID_INTEGRAL_WINDOW > 0)
        // ЭКСПЕРИМЕНТАЛЬНЫЙ РЕЖИМ ИНТЕГРАЛЬНОГО ОКНА
        if (++t >= PID_INTEGRAL_WINDOW) t = 0;  // перемотка t
        integral -= errors[t];                  // вычитаем старое
        errors[t] = error * Ki * _dt_s;         // запоминаем в массив
        integral += errors[t];                  // прибавляем новое
#else
        integral += error * Ki * _dt_s;  // обычное суммирование инт. суммы
#endif

#ifdef PID_OPTIMIZED_I
        // ЭКСПЕРИМЕНТАЛЬНЫЙ РЕЖИМ ОГРАНИЧЕНИЯ ИНТЕГРАЛЬНОЙ СУММЫ
        output = constrain(output, _minOut, _maxOut);
        if (Ki != 0) integral = constrain(integral, (_minOut - output) / (Ki * _dt_s), (_maxOut - output) / (Ki * _dt_s));
#endif

        if (_mode) integral += delta_input * Kp;           // режим пропорционально скорости
        integral = constrain(integral, _minOut, _maxOut);  // ограничиваем инт. сумму
        output += integral;                                // интегральная составляющая
        output = constrain(output, _minOut, _maxOut);      // ограничиваем выход
        return output;
    }

    // возвращает новое значение не ранее, чем через dt миллисекунд (встроенный таймер с периодом dt)
    pidtype getResultTimer() {
        if (millis() - pidTimer >= _dt) {
            pidTimer = millis();
            getResult();
        }
        return output;
    }

    // посчитает выход по реальному прошедшему времени между вызовами функции
    pidtype getResultNow() {
        setDt(millis() - pidTimer);
        pidTimer = millis();
        return getResult();
    }

   private:
    uint32_t _dt = 100;  // время итерации в мс
    float _dt_s = 0.1;   // время итерации в с
    bool _mode = 0, _direction = 0;
    int16_t _minOut = 0, _maxOut = 255;
    pidtype prevInput = 0;
    uint32_t pidTimer = 0;
#if (PID_INTEGRAL_WINDOW > 0)
    pidtype errors[PID_INTEGRAL_WINDOW];
    uint16_t t = 0;
#endif
};
#endif