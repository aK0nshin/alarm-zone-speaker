#include "AlarmManager.h"

AlarmManager::AlarmManager(TrackPlayer& player) : _player(player) {}

AlarmManager::~AlarmManager() {
    if (_queue) vQueueDelete(_queue);
}

bool AlarmManager::begin() {
    if (!_queue) {
        _queue = xQueueCreateStatic(QUEUE_SIZE, sizeof(Event), _queueStorage, &_queueControl);
    }
    if (!_queue) return false;
    clearPipeline();
    return true;
}

bool AlarmManager::enqueue(Event event) {
    if (!_queue) return false;
    if (xQueueSend(_queue, &event, 0) == pdPASS) return true;
    Serial.printf("[Manager] Queue full, event rejected: %u\n", event.number);
    return false;
}

void AlarmManager::playStartupTrack() {
    enqueue({EventKind::STARTUP, 0});
}

void AlarmManager::processCommand(RadioCommand command) {
    if (!_queue || command == RadioCommand::NONE) return;
    if (command == RadioCommand::EMERGENCY_STOP) {
        clearPipeline();
        Serial.println("[Manager] Stopped; pending events cleared");
        return;
    }

    int zone = static_cast<int>(command);
    if (zone < 1 || zone > 6) return;
    if (!enqueue({EventKind::ALARM, static_cast<uint8_t>(zone)})) return;
    if (!_active || _current.kind != EventKind::BACKGROUND) return;
    _player.stop();
    finishEvent();
}

void AlarmManager::tick() {
    if (!_queue) return;
    _player.tick();
    if (_player.isPlaying()) return;

    if (_active) {
        if (_player.hasError()) {
            Serial.println("[Manager] Audio error; entire current event abandoned");
            _player.stop();
            finishEvent();
        } else {
            uint8_t trackCount = _current.kind == EventKind::ALARM ? 3 : 1;
            if (_step < trackCount) {
                playNextTrack();
                return;
            }
            finishEvent();
        }
    }

    if (xQueueReceive(_queue, &_current, 0) == pdPASS) {
        _active = true;
        _step = 0;
        playNextTrack();
        return;
    }
    if (millis() - _lastBackgroundPlayTime < _nextBackgroundInterval) return;
    _current = {EventKind::BACKGROUND, static_cast<uint8_t>(random(1, TOTAL_PHRASES + 1))};
    _active = true;
    _step = 0;
    playNextTrack();
}

void AlarmManager::playNextTrack() {
    char path[24];
    const char* track = nullptr;
    if (_current.kind == EventKind::STARTUP) {
        track = "/territory.wav";
    } else if (_current.kind == EventKind::BACKGROUND) {
        snprintf(path, sizeof(path), "/phrase_%02u.wav", _current.number);
        track = path;
    } else if (_step == 0) {
        track = "/camera.wav";
    } else if (_step == 1) {
        snprintf(path, sizeof(path), "/%02u.wav", _current.number);
        track = path;
    } else {
        track = "/intruder.wav";
    }

    if (!_player.play(track)) {
        Serial.printf("[Manager] Event abandoned: %s\n", track);
        _player.stop();
        finishEvent();
        return;
    }
    ++_step;
}

void AlarmManager::finishEvent() {
    _active = false;
    _step = 0;
    resetBackgroundTimer();
}

void AlarmManager::resetBackgroundTimer() {
    _lastBackgroundPlayTime = millis();
    _nextBackgroundInterval = random(BG_MIN_INTERVAL_MS, BG_MAX_INTERVAL_MS + 1);
}

void AlarmManager::clearPipeline() {
    _player.stop();
    xQueueReset(_queue);
    finishEvent();
}
