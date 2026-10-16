#include "RadioReceiver.h"
#include "RadioDriver.h"
#include "Config.h"

RadioDriver hardwareDriver(Config::RADIO_CE_PIN, Config::RADIO_CSN_PIN);

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
    Serial.printf("[Radio] Broadcast ready, group=%s\n", Config::RADIO_GROUP_NAME);
    return true;
}

RadioCommand RadioReceiver::checkPayload() {
    if (!_ready) return RadioCommand::NONE;
    RadioProtocol::Packet packet;
    if (!hardwareDriver.readPacket(packet.data(), packet.size())) return RadioCommand::NONE;
    RadioCommand command = RadioProtocol::decode(packet.data(), packet.size(), Config::RADIO_GROUP_NAME);
    if (!_duplicates.accept(command, millis(), Config::RADIO_CAMERA_COOLDOWN_MS)) return RadioCommand::NONE;
    Serial.printf("[Radio] command=%u\n", unsigned(command));
    return command;
}
