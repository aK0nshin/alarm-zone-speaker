#include "RadioDriver.h"
#include <SPI.h>

// Аппаратные пины SPI для ESP32-C3 Super Mini
#define SPI_SCK      4
#define SPI_MISO     5
#define SPI_MOSI     6
#define SPI_SS       2

RadioDriver::RadioDriver(uint8_t cePin, uint8_t csnPin) : _radio(cePin, csnPin) {}
RadioDriver::~RadioDriver() {}

bool RadioDriver::begin() {
    SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, SPI_SS); 

    if (!_radio.begin()) return false;

    _radio.setPALevel(RF24_PA_LOW); 
    _radio.setDataRate(RF24_1MBPS);
    _radio.setPayloadSize(1);
    _radio.openReadingPipe(1, (const uint8_t*)"ALARM");
    _radio.startListening();

    return true;
}


bool RadioDriver::readByte(uint8_t &outputByte) {
    if (!_radio.available()) return false;

    _radio.read(&outputByte, 1);
    return true;
}
