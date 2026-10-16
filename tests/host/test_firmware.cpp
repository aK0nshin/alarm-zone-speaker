#include "AlarmManager.h"
#include "Mp3Player.h"
#include <cassert>
#include <string>
#include <vector>
#include <iostream>

static std::vector<uint8_t> pcm;
static bool outputFailure = false;
bool PdmOutput::begin(uint32_t rate) {
    _sampleRate = rate;
    _installed = true;
    pcm.clear();
    return true;
}
void PdmOutput::end() { _installed = false; }
size_t PdmOutput::write(const uint8_t* data, size_t length) {
    if (outputFailure) return 0;
    pcm.insert(pcm.end(), data, data + length);
    return length;
}
uint32_t PdmOutput::drainTimeMs() const { return 80; }

class FakePlayer : public TrackPlayer {
public:
    std::vector<std::string> tracks;
    std::string rejected;
    bool playing = false;
    bool error = false;
    bool play(const char* path) override {
        tracks.emplace_back(path);
        error = rejected == path;
        playing = !error;
        return playing;
    }
    void stop() override { playing = false; error = false; }
    void tick() override {}
    bool isPlaying() const override { return playing; }
    bool hasError() const override { return error; }
    void complete() { playing = false; }
};

static void testManager() {
    testMillis = 0;
    FakePlayer player;
    AlarmManager manager(player);
    assert(manager.begin());
    manager.playStartupTrack();
    manager.tick();
    assert(player.tracks.back() == "/territory.mp3");
    manager.processCommand(RadioCommand::ZONE_2_ALARM);
    manager.tick();
    assert(player.tracks.size() == 1);
    player.complete();
    manager.tick();
    assert(player.tracks.back() == "/camera.mp3");
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/02.mp3");
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/intruder.mp3");
    player.complete(); manager.tick();

    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.mp3");
    player.complete(); manager.tick();
    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.mp3");
    size_t before = player.tracks.size();
    manager.processCommand(RadioCommand::ZONE_3_ALARM);
    assert(!player.playing);
    manager.tick();
    assert(player.tracks.size() == before + 1 && player.tracks.back() == "/camera.mp3");

    manager.processCommand(RadioCommand::ZONE_4_ALARM);
    manager.processCommand(RadioCommand::EMERGENCY_STOP);
    before = player.tracks.size();
    manager.tick();
    assert(!player.playing && player.tracks.size() == before);
    manager.processCommand(static_cast<RadioCommand>(77));
    manager.tick();
    assert(player.tracks.size() == before);

    for (int i = 0; i < 7; ++i) manager.processCommand(RadioCommand::ZONE_1_ALARM);
    for (int i = 0; i < 19; ++i) { player.complete(); manager.tick(); }
    assert(player.tracks.size() == before + 18);
    for (size_t i = before; i < player.tracks.size(); i += 3) {
        assert(player.tracks[i] == "/camera.mp3");
        assert(player.tracks[i + 1] == "/01.mp3");
        assert(player.tracks[i + 2] == "/intruder.mp3");
    }

    player.rejected = "/02.mp3";
    manager.processCommand(RadioCommand::ZONE_2_ALARM);
    manager.processCommand(RadioCommand::ZONE_5_ALARM);
    manager.tick();
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/02.mp3");
    manager.tick();
    assert(player.tracks.back() == "/camera.mp3");
    player.error = true;
    player.complete(); manager.tick();
    before = player.tracks.size();
    manager.tick();
    assert(player.tracks.size() == before);

    testMillis = UINT32_MAX - 100;
    manager.processCommand(RadioCommand::EMERGENCY_STOP);
    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.mp3");
    std::cout << "Manager: startup, FIFO, overflow, preemption, stop, repeat, errors, wrap passed\n";
}

static std::vector<uint8_t> readAsset(const char* path) {
    std::ifstream input(std::string("data/") + path, std::ios::binary);
    assert(input);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), {});
}

static void testMp3(const char* referenceDirectory) {
    Mp3Player player(GPIO_NUM_3);
    assert(player.begin());
    const char* names[] = {"01", "02", "03", "04", "05", "06",
                          "camera", "intruder", "territory", "phrase_01"};
    for (const auto* name : names) {
        std::string path = std::string("/") + name + ".mp3";
        assert(player.play(path.c_str()));
        for (int i = 0; i < 20000 && player.isPlaying(); ++i) {
            player.tick();
            testMillis += 1;
        }
        assert(!player.isPlaying() && !player.hasError());
        std::ifstream reference(std::string(referenceDirectory) + "/" + name + ".pcm", std::ios::binary);
        assert(reference);
        std::vector<uint8_t> expected((std::istreambuf_iterator<char>(reference)), {});
        assert(pcm.size() == expected.size() && !pcm.empty());
        uint64_t difference = 0;
        for (size_t i = 0; i < pcm.size(); i += 2) {
            int16_t actual, wanted;
            std::memcpy(&actual, pcm.data() + i, 2);
            std::memcpy(&wanted, expected.data() + i, 2);
            int delta = std::abs(int(actual) - int(wanted));
            assert(delta < 64);
            difference += delta;
        }
        assert(difference < 4 * (pcm.size() / 2));
    }
    assert(!player.play(nullptr) && player.hasError());
    assert(!player.play("/missing.mp3") && player.hasError());
    testFiles["/bad.mp3"] = {1, 2, 3};
    assert(!player.play("/bad.mp3"));
    auto unsupported = readAsset("01.mp3");
    unsupported[3] &= 0x3f;
    testFiles["/stereo.mp3"] = unsupported;
    assert(!player.play("/stereo.mp3"));
    auto tagged = readAsset("01.mp3");
    tagged.insert(tagged.begin(), {'I', 'D', '3', 4, 0, 0, 0, 0, 0, 0});
    testFiles["/tagged.mp3"] = tagged;
    assert(!player.play("/tagged.mp3"));
    auto shortHeader = readAsset("01.mp3");
    shortHeader.resize(4);
    testFiles["/header.mp3"] = shortHeader;
    assert(!player.play("/header.mp3"));
    auto input = readAsset("territory.mp3");
    input.resize(input.size() - 10);
    testFiles["/truncated.mp3"] = input;
    assert(player.play("/truncated.mp3"));
    for (int i = 0; i < 20000 && player.isPlaying(); ++i) {
        player.tick();
        ++testMillis;
    }
    assert(!player.isPlaying() && player.hasError());
    assert(player.play("/territory.mp3"));
    player.tick();
    outputFailure = true;
    player.tick();
    assert(!player.isPlaying() && player.hasError());
    outputFailure = false;
    assert(player.play("/camera.mp3"));
    player.tick();
    player.stop();
    assert(!player.isPlaying() && !player.hasError());
    assert(player.play("/01.mp3"));
    player.stop();

    FakePlayer fake;
    AlarmManager manager(fake);
    assert(manager.begin());
    manager.playStartupTrack(); manager.tick();
    assert(fake.tracks.back() == "/territory.mp3");
    fake.complete(); manager.tick();
    manager.processCommand(RadioCommand::ZONE_6_ALARM); manager.tick();
    assert(fake.tracks.back() == "/camera.mp3");
    fake.complete(); manager.tick();
    assert(fake.tracks.back() == "/06.mp3");
    fake.complete(); manager.tick();
    assert(fake.tracks.back() == "/intruder.mp3");
    fake.complete(); manager.tick();
    testMillis += 240000; manager.tick();
    assert(fake.tracks.back() == "/phrase_01.mp3");
    manager.processCommand(RadioCommand::EMERGENCY_STOP);
    assert(!fake.playing);
    std::cout << "MP3: 10 assets match FFmpeg; errors, restart, output failure and manager paths passed\n";
}

int main(int argc, char** argv) {
    assert(argc == 2);
    testManager();
    testMp3(argv[1]);
}
