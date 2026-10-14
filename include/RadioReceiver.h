#ifndef RADIO_RECEIVER_H
#define RADIO_RECEIVER_H

#include <Arduino.h>

// Перечисление системных команд, которые могут прийти по радио
enum class RadioCommand {
    NONE,           // Ничего не пришло
    ZONE_1_ALARM,   // Тревога в зоне 1
    ZONE_2_SERVICE, // Сервисное событие в зоне 2
    EMERGENCY_STOP  // Команда полной остановки
};

class RadioReceiver {
public:
    // Конструктор принимает пины или параметры, если это необходимо
    RadioReceiver();
    ~RadioReceiver();

    // Инициализация радиомодуля (вызывается в setup)
    bool begin();

    // Опрос эфира (вызывается в loop). Возвращает команду, если она пришла
    RadioCommand checkPayload();
};

#endif // RADIO_RECEIVER_H
