#pragma once

#include <RF24.h>
#include "RadioProtocol.h"

namespace RadioProtocol {
// Call after RF24::begin(). Broadcast receivers must not send simultaneous ACKs.
inline bool configureRadio(RF24& radio) {
    radio.setChannel(CHANNEL);
    radio.setAddressWidth(ADDRESS_WIDTH);
    radio.disableDynamicPayloads();
    radio.setPayloadSize(PAYLOAD_SIZE);
    radio.setAutoAck(false);
    radio.setCRCLength(RF24_CRC_16);
    return radio.setDataRate(RF24_1MBPS);
}

inline bool configureReceiver(RF24& radio) {
    if (!configureRadio(radio)) return false;
    radio.openReadingPipe(RECEIVE_PIPE, ADDRESS);
    radio.startListening();
    return true;
}

inline bool configureTransmitter(RF24& radio) {
    if (!configureRadio(radio)) return false;
    radio.stopListening();
    radio.openWritingPipe(ADDRESS);
    return true;
}
}
