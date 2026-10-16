#include "RadioReceiver.h"
#include "RadioDriver.h"
#include <RadioSettings.h>

RadioDriver hardwareDriver(10, 7); // CE=10, CSN=7

RadioReceiver::RadioReceiver() {}
RadioReceiver::~RadioReceiver() {}

bool RadioReceiver::begin() {
    _ready = false;
    _duplicates.reset();
    if (!hardwareDriver.begin()) {
        Serial.println("[Radio] Hardware initialization failed");
        return false;
    }
    _ready = true;
    Serial.printf("[Radio] Broadcast ready, group=%s\n", RADIO_GROUP_NAME);
    return true;
}

RadioCommand RadioReceiver::checkPayload() {
    if (!_ready) return RadioCommand::NONE;
    RadioProtocol::Packet packet;
    if (!hardwareDriver.readPacket(packet.data(), packet.size())) return RadioCommand::NONE;
    RadioCommand command = RadioProtocol::decode(packet.data(), packet.size(), RADIO_GROUP_NAME);
    if (!_duplicates.accept(command, millis(), RADIO_CAMERA_COOLDOWN_MS)) return RadioCommand::NONE;
    Serial.printf("[Radio] command=%u\n", unsigned(command));
    return command;
}
