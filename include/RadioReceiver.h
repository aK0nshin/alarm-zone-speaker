#ifndef RADIO_RECEIVER_H
#define RADIO_RECEIVER_H

#include <RadioProtocol.h>

class RadioReceiver {
public:
    RadioReceiver();
    ~RadioReceiver();

    bool begin();

    RadioCommand checkPayload();
private:
    RadioProtocol::DuplicateFilter _duplicates;
    bool _ready = false;
};

#endif // RADIO_RECEIVER_H
