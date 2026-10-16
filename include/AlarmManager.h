#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "TrackPlayer.h"
#include <RadioProtocol.h>

class AlarmManager {
public:
    explicit AlarmManager(TrackPlayer& player);
    ~AlarmManager();
    AlarmManager(const AlarmManager&) = delete;
    AlarmManager& operator=(const AlarmManager&) = delete;

    bool begin();
    void playStartupTrack();
    void processCommand(RadioCommand command);
    void tick();

private:
    enum class EventKind : uint8_t { STARTUP, ALARM, BACKGROUND };
    struct Event {
        EventKind kind;
        uint8_t number;
    };
    static constexpr uint8_t QUEUE_SIZE = 6;
    static constexpr uint32_t BG_MIN_INTERVAL_MS = 4000; // 240000; // 4 минуты
    static constexpr uint32_t BG_MAX_INTERVAL_MS = 6000; // 360000; // 6 минут
    static constexpr uint8_t TOTAL_PHRASES = 1;

    bool enqueue(Event event);
    void finishEvent();
    void resetBackgroundTimer();
    void clearPipeline();
    void playNextTrack();

    TrackPlayer& _player;
    StaticQueue_t _queueControl = {};
    uint8_t _queueStorage[QUEUE_SIZE * sizeof(Event)] = {};
    QueueHandle_t _queue = nullptr;
    Event _current = {EventKind::STARTUP, 0};
    bool _active = false;
    uint8_t _step = 0;
    uint32_t _lastBackgroundPlayTime = 0;
    uint32_t _nextBackgroundInterval = 0;
};
