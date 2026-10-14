#ifndef RADIO_DRIVER_H
#define RADIO_DRIVER_H

#include <Arduino.h>
#include <RF24.h>

class RadioDriver {
public:
    RadioDriver(uint8_t cePin, uint8_t csnPin);
    ~RadioDriver();

    // Инициализация физической шины SPI и чипа nRF24L01+
    bool begin();

    bool readByte(uint8_t &outputByte);

private:
    RF24 _radio;
};

#endif // RADIO_DRIVER_H
