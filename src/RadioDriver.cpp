#include "RadioDriver.h"
#include "Config.h"
#include <RadioProtocolRF24.h>
#include <SPI.h>

RadioDriver::RadioDriver(uint8_t cePin, uint8_t csnPin) : _radio(cePin, csnPin) {}
RadioDriver::~RadioDriver() {}

bool RadioDriver::begin() {
    SPI.begin(Config::RADIO_SCK_PIN, Config::RADIO_MISO_PIN,
              Config::RADIO_MOSI_PIN, Config::RADIO_SS_PIN);

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
