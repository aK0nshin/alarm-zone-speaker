#ifndef RADIO_RECEIVER_H
#define RADIO_RECEIVER_H

#include <Arduino.h>

// Что может прийти по радио
enum class RadioCommand {
    NONE = 0,
    ZONE_1_ALARM = 1,
    ZONE_2_ALARM = 2,
    ZONE_3_ALARM = 3,
    ZONE_4_ALARM = 4,
    ZONE_5_ALARM = 5,
    ZONE_6_ALARM = 6,
    EMERGENCY_STOP = 99
};

class RadioReceiver {
public:
    RadioReceiver();
    ~RadioReceiver();

    bool begin();

    RadioCommand checkPayload();
};

#endif // RADIO_RECEIVER_H
