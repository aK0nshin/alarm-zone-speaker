#include "AlarmManager.h"
#include "WavPlayer.h"
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
    assert(player.tracks.back() == "/territory.wav");
    manager.processCommand(RadioCommand::ZONE_2_ALARM);
    manager.tick();
    assert(player.tracks.size() == 1);
    player.complete();
    manager.tick();
    assert(player.tracks.back() == "/camera.wav");
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/02.wav");
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/intruder.wav");
    player.complete(); manager.tick();

    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.wav");
    player.complete(); manager.tick();
    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.wav");
    size_t before = player.tracks.size();
    manager.processCommand(RadioCommand::ZONE_3_ALARM);
    assert(!player.playing);
    manager.tick();
    assert(player.tracks.size() == before + 1 && player.tracks.back() == "/camera.wav");

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
        assert(player.tracks[i] == "/camera.wav");
        assert(player.tracks[i + 1] == "/01.wav");
        assert(player.tracks[i + 2] == "/intruder.wav");
    }

    player.rejected = "/02.wav";
    manager.processCommand(RadioCommand::ZONE_2_ALARM);
    manager.processCommand(RadioCommand::ZONE_5_ALARM);
    manager.tick();
    player.complete(); manager.tick();
    assert(player.tracks.back() == "/02.wav");
    manager.tick();
    assert(player.tracks.back() == "/camera.wav");
    player.error = true;
    player.complete(); manager.tick();
    before = player.tracks.size();
    manager.tick();
    assert(player.tracks.size() == before);

    testMillis = UINT32_MAX - 100;
    manager.processCommand(RadioCommand::EMERGENCY_STOP);
    testMillis += 240000;
    manager.tick();
    assert(player.tracks.back() == "/phrase_01.wav");
    std::cout << "Manager: startup, FIFO, overflow, preemption, stop, repeat, errors, wrap passed\n";
}

static std::vector<uint8_t> readAsset(const char* path) {
    std::ifstream input(std::string("data/") + path, std::ios::binary);
    assert(input);
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), {});
}

static void testAudio() {
    WavPlayer player(GPIO_NUM_3);
    assert(player.begin());
    const char* names[] = {"01.wav", "02.wav", "03.wav", "04.wav", "05.wav", "06.wav",
                          "camera.wav", "intruder.wav", "territory.wav", "phrase_01.wav"};
    for (const auto* name : names) {
        auto input = readAsset(name);
        drwav reference = {};
        assert(drwav_init_memory(&reference, input.data(), input.size(), nullptr));
        const size_t offset = reference.dataChunkDataPos;
        const size_t samples = reference.totalPCMFrameCount;
        drwav_uninit(&reference);
        assert(player.play((std::string("/") + name).c_str()));
        for (int i = 0; i < 10000 && player.isPlaying(); ++i) {
            player.tick();
            testMillis += 16;
        }
        assert(!player.isPlaying() && !player.hasError());
        assert(pcm.size() == samples * 2);
        for (size_t i = 0; i < samples; ++i) {
            int16_t actual;
            std::memcpy(&actual, pcm.data() + 2 * i, 2);
            assert(actual == (int(input[offset + i]) - 128) * 256);
        }
    }
    assert(!player.play("/missing.wav") && player.hasError());
    assert(!player.play(nullptr) && player.hasError());
    testFiles["/broken.wav"] = {1, 2, 3};
    assert(!player.play("/broken.wav"));
    auto truncated = readAsset("territory.wav");
    truncated.resize(100);
    testFiles["/truncated.wav"] = truncated;
    assert(!player.play("/truncated.wav"));

    auto unsupported = readAsset("01.wav");
    unsupported[22] = 2;
    testFiles["/stereo.wav"] = unsupported;
    assert(!player.play("/stereo.wav"));

    const int16_t expected[] = {-32768, -1, 0, 32767};
    drwav_data_format format = {};
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = 16000;
    format.bitsPerSample = 16;
    drwav writer = {};
    void* encoded = nullptr;
    size_t encodedSize = 0;
    assert(drwav_init_memory_write(&writer, &encoded, &encodedSize, &format, nullptr));
    assert(drwav_write_pcm_frames(&writer, 4, expected) == 4);
    drwav_uninit(&writer);
    auto* first = static_cast<uint8_t*>(encoded);
    testFiles["/pcm16.wav"] = std::vector<uint8_t>(first, first + encodedSize);
    drwav_free(encoded, nullptr);
    assert(player.play("/pcm16.wav"));
    player.tick();
    assert(pcm.size() == sizeof(expected));
    assert(std::memcmp(pcm.data(), expected, sizeof(expected)) == 0);

    assert(player.play("/territory.wav"));
    player.tick();
    player.stop();
    assert(!player.isPlaying() && !player.hasError());
    assert(player.play("/camera.wav"));
    outputFailure = true;
    player.tick();
    assert(!player.isPlaying() && player.hasError());
    outputFailure = false;

    assert(player.play("/01.wav"));
    for (int i = 0; i < 63; ++i) player.tick();
    assert(player.isPlaying());
    testMillis += 79;
    player.tick();
    assert(player.isPlaying());
    ++testMillis;
    player.tick();
    assert(!player.isPlaying());
    std::cout << "Audio: 10 assets decoded sample-for-sample; PCM16, errors, restart, DMA drain passed\n";
}

int main() {
    testManager();
    testAudio();
}
