#include "RadioReceiver.h"

RadioReceiver::RadioReceiver() {}
RadioReceiver::~RadioReceiver() {}

bool RadioReceiver::begin() {
    Serial.println("[Radio] Модуль связи успешно инициализирован.");
    return true;
}

RadioCommand RadioReceiver::checkPayload() {
    if (!Serial.available()) return RadioCommand::NONE; // Ранний выход: эфир пуст

    char incomingByte = Serial.read();
    
    switch (incomingByte) {
        case '1': 
            Serial.println("[Radio] Поймали: Тревога Зоны 1");
            return RadioCommand::ZONE_1_ALARM; // Ранний выход из метода при совпадении
        case '2': 
            Serial.println("[Radio] Поймали: Сервис Зоны 2");
            return RadioCommand::ZONE_2_SERVICE; // Ранний выход
        case 's': 
            Serial.println("[Radio] Поймали: ЭКСТРЕННЫЙ СТОП");
            return RadioCommand::EMERGENCY_STOP; // Ранний выход
        default:
            return RadioCommand::NONE; // Ранний выход для неизвестного мусора
    }
}
