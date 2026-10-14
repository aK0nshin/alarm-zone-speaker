#include "RadioReceiver.h"
#include "RadioDriver.h"

RadioDriver hardwareDriver(10, 7); // CE=10, CSN=7

RadioReceiver::RadioReceiver() {}
RadioReceiver::~RadioReceiver() {}

bool RadioReceiver::begin() {
    if (!hardwareDriver.begin()) {
        Serial.println("[RadioReceiver] Ошибка: Железо радиомодуля не отвечает.");
        return false;
    }

    Serial.println("[RadioReceiver] Система связи готова к обработке команд.");
    return true;
}

RadioCommand RadioReceiver::checkPayload() {
    uint8_t rawByte = 0;
    
    if (!hardwareDriver.readByte(rawByte)) return RadioCommand::NONE;

    switch (rawByte) {
        case 1:   case '1': return RadioCommand::ZONE_1_ALARM;
        case 2:   case '2': return RadioCommand::ZONE_2_ALARM;
        case 3:   case '3': return RadioCommand::ZONE_3_ALARM;
        case 4:   case '4': return RadioCommand::ZONE_4_ALARM;
        case 5:   case '5': return RadioCommand::ZONE_5_ALARM;
        case 6:   case '6': return RadioCommand::ZONE_6_ALARM;
        case 7:   case '7': return RadioCommand::ZONE_7_ALARM;
        case 8:   case '8': return RadioCommand::ZONE_8_ALARM;
        case 9:   case '9': return RadioCommand::ZONE_9_ALARM;
        case 10:   case '10': return RadioCommand::ZONE_10_ALARM;
        case 's': case 99:  return RadioCommand::EMERGENCY_STOP;
        default:            return RadioCommand::NONE;
    }
}
