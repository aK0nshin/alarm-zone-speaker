#pragma once

#include <SPIFFS.h>
#include <libhelix-mp3/mp3dec.h>
#include "PdmOutput.h"
#include "TrackPlayer.h"

class Mp3Player : public TrackPlayer {
public:
    explicit Mp3Player(gpio_num_t pin) : _output(pin) {}
    ~Mp3Player() override { stop(); }
    bool begin();
    bool play(const char* path) override;
    void stop() override;
    void tick() override;
    bool isPlaying() const override { return _playing; }
    bool hasError() const override { return _failed; }

private:
    bool fillInput();
    bool validFrame();
    void fail(const char* reason);

    PdmOutput _output;
    fs::File _file;
    HMP3Decoder _decoder = nullptr;
    uint8_t _input[2048] = {};
    // Выходной буфер Helix должен вмещать и стереоданные при некорректном заголовке.
    int16_t _pcm[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP] = {};
    int _inputSize = 0;
    size_t _pcmCount = 0;
    size_t _pcmOffset = 0;
    uint32_t _frames = 0;
    uint32_t _maxDecodeUs = 0;
    uint32_t _maxLoadUs = 0;
    uint32_t _drainStarted = 0;
    bool _playing = false;
    bool _draining = false;
    bool _failed = false;
};
