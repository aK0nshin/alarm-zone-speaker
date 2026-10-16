#define DR_WAV_IMPLEMENTATION
#define DR_WAV_NO_STDIO
#include "WavPlayer.h"

WavPlayer::WavPlayer(gpio_num_t pin) : _output(pin) {}

WavPlayer::~WavPlayer() {
    stop();
}

bool WavPlayer::begin() {
    bool ready = _output.begin(8000);
    _output.end();
    return ready;
}

size_t WavPlayer::readFile(void* context, void* output, size_t count) {
    return static_cast<fs::File*>(context)->read(static_cast<uint8_t*>(output), count);
}

drwav_bool32 WavPlayer::seekFile(void* context, int offset, drwav_seek_origin origin) {
    auto& file = *static_cast<fs::File*>(context);
    int64_t base = 0;
    if (origin == DRWAV_SEEK_CUR) base = file.position();
    if (origin == DRWAV_SEEK_END) base = file.size();
    int64_t position = base + offset;
    if (position < 0 || uint64_t(position) > file.size()) return DRWAV_FALSE;
    return file.seek(static_cast<uint32_t>(position)) ? DRWAV_TRUE : DRWAV_FALSE;
}

bool WavPlayer::play(const char* path) {
    stop();
    if (!path) {
        fail("Empty path");
        return false;
    }
    _file = SPIFFS.open(path, "r");
    if (!_file || _file.isDirectory()) {
        Serial.printf("[Audio] Cannot open %s\n", path);
        fail("File unavailable");
        return false;
    }
    // No onTell: retain the declared data length and reject truncation below.
    _decoderReady = drwav_init(&_decoder, readFile, seekFile, nullptr, &_file, nullptr);
    if (!_decoderReady) {
        fail("Invalid WAV header");
        return false;
    }
    if (_decoder.translatedFormatTag != DR_WAVE_FORMAT_PCM ||
        _decoder.channels != 1 ||
        (_decoder.bitsPerSample != 8 && _decoder.bitsPerSample != 16) ||
        _decoder.sampleRate < 8000 || _decoder.sampleRate > 48000 ||
        _decoder.totalPCMFrameCount == 0 ||
        _decoder.fmt.blockAlign != _decoder.bitsPerSample / 8 ||
        _decoder.dataChunkDataSize % _decoder.fmt.blockAlign != 0 ||
        _decoder.dataChunkDataPos > _file.size() ||
        _decoder.dataChunkDataSize > _file.size() - _decoder.dataChunkDataPos) {
        fail("Unsupported or truncated WAV");
        return false;
    }
    if (!_output.begin(_decoder.sampleRate)) {
        fail("Cannot start playback");
        return false;
    }
    _remaining = _decoder.totalPCMFrameCount;
    _playing = true;
    Serial.printf("[Audio] Playing %s\n", path);
    return true;
}

void WavPlayer::stop() {
    _playing = false;
    _draining = false;
    _failed = false;
    _remaining = 0;
    if (_decoderReady) {
        drwav_uninit(&_decoder);
        _decoderReady = false;
    }
    if (_file) _file.close();
    _output.end();
}

void WavPlayer::fail(const char* reason) {
    stop();
    _failed = true;
    Serial.printf("[Audio] %s\n", reason);
}

void WavPlayer::tick() {
    if (!_playing) return;
    if (_draining) {
        if (millis() - _drainStarted >= _output.drainTimeMs()) stop();
        return;
    }

    int16_t samples[128];
    drwav_uint64 wanted = _remaining < 128 ? _remaining : 128;
    drwav_uint64 count = drwav_read_pcm_frames_s16(&_decoder, wanted, samples);
    size_t bytes = size_t(count) * sizeof(samples[0]);
    if (count != wanted ||
        _output.write(reinterpret_cast<const uint8_t*>(samples), bytes) != bytes) {
        fail("Playback interrupted");
        return;
    }
    _remaining -= count;
    if (_remaining) return;
    _draining = true;
    _drainStarted = millis();
}
