#include "RadioProtocol.h"
#include <cstring>

namespace RadioProtocol {
namespace {
bool validCommand(RadioCommand command) {
    uint8_t value = static_cast<uint8_t>(command);
    return (value >= 1 && value <= 6) || command == RadioCommand::EMERGENCY_STOP;
}

size_t groupLength(const char* group) {
    if (!group) return 0;
    for (size_t i = 0; i < GROUP_SIZE; ++i) {
        if (!group[i]) return i;
    }
    return 0;
}
}

bool encode(RadioCommand command, const char* group, Packet& output) {
    size_t length = groupLength(group);
    if (!length || !validCommand(command)) return false;
    Packet packet = {};
    packet[0] = 'A';
    packet[1] = 'Z';
    packet[2] = VERSION;
    packet[3] = static_cast<uint8_t>(command);
    std::memcpy(packet.data() + 4, group, length);
    output = packet;
    return true;
}

RadioCommand decode(const uint8_t* payload, size_t length, const char* group) {
    size_t nameLength = groupLength(group);
    if (!payload || length != PAYLOAD_SIZE || !nameLength) return RadioCommand::NONE;
    if (payload[0] != 'A' || payload[1] != 'Z' || payload[2] != VERSION) return RadioCommand::NONE;
    if (std::memcmp(payload + 4, group, nameLength) != 0) return RadioCommand::NONE;
    for (size_t i = nameLength; i < GROUP_SIZE; ++i) {
        if (payload[4 + i] != 0) return RadioCommand::NONE;
    }
    RadioCommand command = static_cast<RadioCommand>(payload[3]);
    return validCommand(command) ? command : RadioCommand::NONE;
}

bool DuplicateFilter::accept(RadioCommand command, uint32_t nowMs, uint32_t intervalMs) {
    if (command == RadioCommand::EMERGENCY_STOP) return true;
    unsigned zone = static_cast<unsigned>(command);
    if (zone < 1 || zone > 6) return false;
    size_t index = zone - 1;
    if (_seen[index] && uint32_t(nowMs - _lastAccepted[index]) < intervalMs) return false;
    _seen[index] = true;
    _lastAccepted[index] = nowMs;
    return true;
}

void DuplicateFilter::reset() {
    for (auto& seen : _seen) seen = false;
}
}
