#include "RadioDriver.h"
#include <RadioProtocolRF24.h>
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
    return RadioProtocol::configureReceiver(_radio);
}


bool RadioDriver::readPacket(uint8_t* output, size_t length) {
    if (!output || length != RadioProtocol::PAYLOAD_SIZE) return false;
    if (!_radio.available()) return false;

    _radio.read(output, length);
    return true;
}
