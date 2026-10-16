#pragma once

#include <SPIFFS.h>
#include <dr_wav.h>
#include "PdmOutput.h"
#include "TrackPlayer.h"

class WavPlayer : public TrackPlayer {
public:
    explicit WavPlayer(gpio_num_t pin);
    ~WavPlayer() override;
    bool begin();
    bool play(const char* path) override;
    void stop() override;
    void tick() override;
    bool isPlaying() const override { return _playing; }
    bool hasError() const override { return _failed; }

private:
    void fail(const char* reason);
    static size_t readFile(void* context, void* output, size_t count);
    static drwav_bool32 seekFile(void* context, int offset, drwav_seek_origin origin);
    PdmOutput _output;
    drwav _decoder = {};
    fs::File _file;
    drwav_uint64 _remaining = 0;
    bool _decoderReady = false;
    uint32_t _drainStarted = 0;
    bool _playing = false;
    bool _draining = false;
    bool _failed = false;
};
