#ifndef RADIO_RECEIVER_H
#define RADIO_RECEIVER_H

#include <Arduino.h>

// Что может прийти по радио
enum class RadioCommand {
    NONE,
    ZONE_1_ALARM,
    ZONE_2_ALARM,
    ZONE_3_ALARM,
    ZONE_4_ALARM,
    ZONE_5_ALARM,
    ZONE_6_ALARM,
    ZONE_7_ALARM,
    ZONE_8_ALARM,
    ZONE_9_ALARM,
    ZONE_10_ALARM,
    EMERGENCY_STOP
};

class RadioReceiver {
public:
    RadioReceiver();
    ~RadioReceiver();

    bool begin();

    RadioCommand checkPayload();
};

#endif // RADIO_RECEIVER_H
