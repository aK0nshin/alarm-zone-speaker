#pragma once

#include <array>
#include <stddef.h>
#include <stdint.h>

enum class RadioCommand : uint8_t {
    NONE = 0,
    ZONE_1_ALARM = 1,
    ZONE_2_ALARM = 2,
    ZONE_3_ALARM = 3,
    ZONE_4_ALARM = 4,
    ZONE_5_ALARM = 5,
    ZONE_6_ALARM = 6,
    EMERGENCY_STOP = 99
};

namespace RadioProtocol {
constexpr uint8_t VERSION = 3;
constexpr uint8_t CHANNEL = 76;
constexpr uint8_t ADDRESS_WIDTH = 5;
constexpr uint8_t ADDRESS[ADDRESS_WIDTH] = {'A', 'Z', 'S', 'P', 'K'};
constexpr uint8_t RECEIVE_PIPE = 1;
constexpr size_t GROUP_SIZE = 16;
constexpr uint8_t PAYLOAD_SIZE = 4 + GROUP_SIZE;
using Packet = std::array<uint8_t, PAYLOAD_SIZE>;

bool encode(RadioCommand command, const char* group, Packet& output);
RadioCommand decode(const uint8_t* payload, size_t length, const char* group);

class DuplicateFilter {
public:
    bool accept(RadioCommand command, uint32_t nowMs, uint32_t intervalMs);
    void reset();
private:
    uint32_t _lastAccepted[6] = {};
    bool _seen[6] = {};
};
}
