// FreeRTOSConfig.h
#ifndef FREERTOSCONFIG_H
#define FREERTOSCONFIG_H

// =============================================
// Обязательные настройки для ESP32 (v3.1.3)
// =============================================

#define configUSE_16_BIT_TICKS         0       // Всегда 0 для ESP32
#define configNUMBER_OF_CORES          2       // ESP32 Dual-core

// =============================================
// Базовые настройки планировщика
// =============================================
#define configUSE_PREEMPTION           1       // Вытесняющая многозадачность включена
#define configUSE_TICKLESS_IDLE        0       // Можно включить для энергосбережения
#define configCPU_CLOCK_HZ             (240000000) // Тактовая частота 240 МГц
#define configTICK_RATE_HZ             (1000)      // 1 KHz системный тик
#define configMAX_PRIORITIES           (25)
#define configMINIMAL_STACK_SIZE       ((uint16_t)1024)

// =============================================
// Дополнительные функции
// =============================================
#define configUSE_MUTEXES              1
#define configUSE_TASK_NOTIFICATIONS   1
#define configUSE_IDLE_HOOK            0
#define configUSE_TICK_HOOK            0

// =============================================
// Настройки памяти
// =============================================
#define configTOTAL_HEAP_SIZE          ((size_t)(4 * 1024 * 1024)) // 4MB

// =============================================
// Особенности ESP32
// =============================================
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configSUPPORT_STATIC_ALLOCATION     1

#endif // FREERTOSCONFIG_H
