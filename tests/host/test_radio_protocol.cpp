#include <RadioProtocolRF24.h>
#include <Config.h>
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace RadioProtocol;

static void testCodec() {
    Packet packet = {};
    for (int value = 0; value < 256; ++value) {
        RadioCommand command = static_cast<RadioCommand>(value);
        Packet before = packet;
        bool valid = (value >= 1 && value <= 6) || value == 99;
        assert(encode(command, Config::RADIO_GROUP_NAME, packet) == valid);
        if (!valid) {
            assert(packet == before);
            continue;
        }
        assert(decode(packet.data(), packet.size(), Config::RADIO_GROUP_NAME) == command);
    }
    assert(encode(RadioCommand::ZONE_1_ALARM, "yard", packet));
    const uint8_t expected[PAYLOAD_SIZE] = {'A', 'Z', 3, 1, 'y', 'a', 'r', 'd'};
    assert(std::memcmp(packet.data(), expected, sizeof(expected)) == 0);
    for (const char* group : {"other", "YARD", "yar", "yard2", "", "1234567890123456"}) {
        assert(decode(packet.data(), packet.size(), group) == RadioCommand::NONE);
    }
    assert(decode(nullptr, packet.size(), "yard") == RadioCommand::NONE);
    assert(decode(packet.data(), packet.size(), nullptr) == RadioCommand::NONE);
    for (size_t length : {size_t(0), size_t(1), size_t(19), size_t(21), size_t(32)}) {
        assert(decode(packet.data(), length, "yard") == RadioCommand::NONE);
    }
    for (size_t i : {size_t(0), size_t(1), size_t(2), size_t(4), size_t(19)}) {
        Packet invalid = packet;
        invalid[i] ^= 1;
        assert(decode(invalid.data(), invalid.size(), "yard") == RadioCommand::NONE);
    }
    for (int value = 0; value < 256; ++value) {
        packet[3] = value;
        bool valid = (value >= 1 && value <= 6) || value == 99;
        assert(decode(packet.data(), packet.size(), "yard") ==
               (valid ? static_cast<RadioCommand>(value) : RadioCommand::NONE));
    }
    Packet before = packet;
    assert(!encode(RadioCommand::ZONE_1_ALARM, nullptr, packet));
    assert(!encode(RadioCommand::ZONE_1_ALARM, "", packet));
    assert(!encode(RadioCommand::ZONE_1_ALARM, "1234567890123456", packet));
    assert(packet == before);
    assert(encode(RadioCommand::ZONE_6_ALARM, "123456789012345", packet));
    assert(decode(packet.data(), packet.size(), "123456789012345") == RadioCommand::ZONE_6_ALARM);
}

static void testCooldown() {
    DuplicateFilter filter;
    for (unsigned zone = 1; zone <= 6; ++zone) {
        auto command = static_cast<RadioCommand>(zone);
        assert(filter.accept(command, 0, 30000));
        assert(!filter.accept(command, 1, 30000));
        assert(!filter.accept(command, 29999, 30000));
        assert(filter.accept(command, 30000, 30000));
    }
    assert(filter.accept(RadioCommand::EMERGENCY_STOP, 30001, 30000));
    assert(filter.accept(RadioCommand::EMERGENCY_STOP, 30001, 30000));
    assert(!filter.accept(RadioCommand::ZONE_1_ALARM, 30001, 30000));
    assert(!filter.accept(RadioCommand::NONE, 30001, 30000));
    assert(!filter.accept(static_cast<RadioCommand>(7), 30001, 30000));
    filter.reset();
    assert(filter.accept(RadioCommand::ZONE_1_ALARM, 0xfffffff0, 100));
    assert(!filter.accept(RadioCommand::ZONE_1_ALARM, 20, 100));
    assert(filter.accept(RadioCommand::ZONE_1_ALARM, 84, 100));
    assert(filter.accept(RadioCommand::ZONE_1_ALARM, 84, 0));
    assert(filter.accept(RadioCommand::ZONE_1_ALARM, 84, 0));
}

static void checkConfiguration(const RF24& radio) {
    assert(radio.channel == 76 && radio.addressWidth == 5 && radio.payloadSize == 20);
    assert(!radio.dynamic && !radio.autoAck && radio.crc16 && radio.rateSet);
    assert(std::memcmp(radio.address, "AZSPK", 5) == 0);
}

int main() {
    testCodec();
    testCooldown();
    RF24 receiver, transmitter;
    assert(configureReceiver(receiver));
    assert(configureTransmitter(transmitter));
    checkConfiguration(receiver);
    checkConfiguration(transmitter);
    assert(receiver.listening && receiver.readingOpened && receiver.pipe == 1);
    assert(!transmitter.listening && transmitter.writingOpened);
    RF24 brokenReceiver, brokenTransmitter;
    brokenReceiver.rateSupported = false;
    brokenTransmitter.rateSupported = false;
    assert(!configureReceiver(brokenReceiver));
    assert(!configureTransmitter(brokenTransmitter));
    assert(!brokenReceiver.readingOpened && !brokenReceiver.listening);
    assert(!brokenTransmitter.writingOpened);
    std::puts("Radio protocol: groups, commands, malformed packets, per-camera cooldown, broadcast passed");
}
