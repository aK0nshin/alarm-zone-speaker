#include "Mp3Player.h"
#include <cstring>

bool Mp3Player::begin() {
    bool ready = _output.begin(24000);
    _output.end();
    return ready;
}

bool Mp3Player::fillInput() {
    size_t left = _file.size() - _file.position();
    size_t space = sizeof(_input) - _inputSize;
    size_t wanted = left < space ? left : space;
    if (!wanted) return true;
    size_t count = _file.read(_input + _inputSize, wanted);
    if (count != wanted) {
        fail("File read failed");
        return false;
    }
    _inputSize += count;
    return true;
}

bool Mp3Player::validFrame() {
    // A 576-sample frame at 24 kHz / 64 kbit/s needs at least 192 bytes.
    if (_inputSize < 192 || MP3FindSyncWord(_input, _inputSize) != 0) return false;
    MP3FrameInfo info = {};
    if (MP3GetNextFrameInfo(_decoder, &info, _input) != ERR_MP3_NONE) return false;
    return info.layer == 3 && info.nChans == 1 && info.samprate == 24000 &&
           info.bitrate == 64000 && info.bitsPerSample == 16 && info.outputSamps == 576;
}

bool Mp3Player::play(const char* path) {
    stop();
    if (!path) {
        fail("Empty path");
        return false;
    }
    _file = SPIFFS.open(path, "r");
    if (!_file || _file.isDirectory()) {
        fail("File unavailable");
        return false;
    }
    _decoder = MP3InitDecoder();
    if (!_decoder) {
        fail("Decoder allocation failed");
        return false;
    }
    if (!fillInput()) return false;
    if (!validFrame()) {
        fail("Expected tag-free MP3: mono, 24000 Hz, 64 kbit/s");
        return false;
    }
    if (!_output.begin(24000)) {
        fail("Cannot start output");
        return false;
    }
    _playing = true;
    Serial.printf("[MP3] Playing %s\n", path);
    return true;
}

void Mp3Player::stop() {
    _playing = false;
    _draining = false;
    _failed = false;
    if (_decoder) {
        MP3FreeDecoder(_decoder);
        _decoder = nullptr;
    }
    if (_file) _file.close();
    _output.end();
    if (_frames) {
        Serial.printf("[MP3] frames=%u, max decode=%u us, max read+decode=%u us, frame=24000 us\n",
                      unsigned(_frames), unsigned(_maxDecodeUs), unsigned(_maxLoadUs));
    }
    _inputSize = 0;
    _pcmCount = 0;
    _pcmOffset = 0;
    _frames = 0;
    _maxDecodeUs = 0;
    _maxLoadUs = 0;
}

void Mp3Player::fail(const char* reason) {
    stop();
    _failed = true;
    Serial.printf("[MP3] %s\n", reason);
}

void Mp3Player::tick() {
    if (!_playing) return;
    if (_draining) {
        if (millis() - _drainStarted >= _output.drainTimeMs()) stop();
        return;
    }
    if (_pcmOffset < _pcmCount) {
        size_t count = _pcmCount - _pcmOffset;
        if (count > 128) count = 128;
        size_t bytes = count * sizeof(_pcm[0]);
        if (_output.write(reinterpret_cast<uint8_t*>(_pcm + _pcmOffset), bytes) != bytes) {
            fail("Output write failed");
            return;
        }
        _pcmOffset += count;
        return;
    }

    uint32_t started = micros();
    if (!fillInput()) return;
    if (!_inputSize) {
        _draining = true;
        _drainStarted = millis();
        return;
    }
    if (!validFrame()) {
        fail("Invalid frame or unexpected format change");
        return;
    }

    unsigned char* cursor = _input;
    int remaining = _inputSize;
    uint32_t decodeStarted = micros();
    int result = MP3Decode(_decoder, &cursor, &remaining, _pcm, 0);
    uint32_t elapsed = micros() - decodeStarted;
    if (elapsed > _maxDecodeUs) _maxDecodeUs = elapsed;
    if (result != ERR_MP3_NONE || remaining < 0 || remaining >= _inputSize) {
        Serial.printf("[MP3] Decoder error: %d\n", result);
        fail("Truncated or damaged MP3 frame");
        return;
    }
    MP3FrameInfo info = {};
    MP3GetLastFrameInfo(_decoder, &info);
    if (info.nChans != 1 || info.samprate != 24000 || info.outputSamps != 576) {
        fail("Unexpected decoded format");
        return;
    }
    std::memmove(_input, cursor, remaining);
    _inputSize = remaining;
    _pcmCount = info.outputSamps;
    _pcmOffset = 0;
    ++_frames;
    elapsed = micros() - started;
    if (elapsed > _maxLoadUs) _maxLoadUs = elapsed;
}
