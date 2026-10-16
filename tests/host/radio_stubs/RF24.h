#pragma once
#include <stdint.h>
#include <cstring>

enum rf24_datarate_e { RF24_1MBPS };
enum rf24_crclength_e { RF24_CRC_16 };

class RF24 {
public:
    uint8_t channel = 0, addressWidth = 0, payloadSize = 0, pipe = 0;
    uint8_t address[5] = {};
    bool dynamic = true, autoAck = false, crc16 = false, rateSet = false;
    bool listening = false, readingOpened = false, writingOpened = false;
    bool rateSupported = true;
    void setChannel(uint8_t value) { channel = value; }
    void setAddressWidth(uint8_t value) { addressWidth = value; }
    void disableDynamicPayloads() { dynamic = false; }
    void setPayloadSize(uint8_t value) { payloadSize = value; }
    void setAutoAck(bool value) { autoAck = value; }
    void setCRCLength(rf24_crclength_e value) { crc16 = value == RF24_CRC_16; }
    bool setDataRate(rf24_datarate_e value) {
        rateSet = value == RF24_1MBPS;
        return rateSupported;
    }
    void openReadingPipe(uint8_t value, const uint8_t* bytes) {
        pipe = value;
        std::memcpy(address, bytes, sizeof(address));
        readingOpened = true;
    }
    void openWritingPipe(const uint8_t* bytes) {
        std::memcpy(address, bytes, sizeof(address));
        writingOpened = true;
    }
    void startListening() { listening = true; }
    void stopListening() { listening = false; }
};
