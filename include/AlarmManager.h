#ifndef ALARM_MANAGER_H
#define ALARM_MANAGER_H

#include <Arduino.h>
#include "AudioPlayer.h"
#include "RadioReceiver.h"

enum class AudioPriority {
    NONE,
    NON_IMPORTANT, 
    ALARM          
};

class AlarmManager {
public:
    AlarmManager(AudioPlayer& player);
    
    void begin();
    void processCommand(RadioCommand cmd); 
    void tick();                           

private:
    void addTrackToPipeline(const char* filePath); 
    void playBackgroundPhrase();
    void resetBackgroundTimer();
    void clearPipeline();

    AudioPlayer& _player;
    AudioPriority _currentPriority;

    // --- Единый конвейер воспроизведения треков ---
    static const uint8_t PIPELINE_SIZE = 20;
    String _pipelineQueue[PIPELINE_SIZE];
    uint8_t _pipeHead;
    uint8_t _pipeTail;

    // Настройки фонового таймера фраз
    unsigned long _lastBackgroundPlayTime;
    unsigned long _nextBackgroundInterval;
    
    // =========================================================================
    // НАСТРОЙКИ БИЗНЕС-ЛОГИКИ
    // =========================================================================
    
    // Ограничения зон и спецкоманд
    static const int MIN_ZONE_NUMBER = 1;
    static const int MAX_ZONE_NUMBER = 6;
    static const int EMERGENCY_STOP_CODE = 99; // Команда EMERGENCY_STOP всегда 99

    // Временные интервалы фоновых фраз (в миллисекундах)
    const uint32_t BG_MIN_INTERVAL_MS = 240000; // 4 минуты
    const uint32_t BG_MAX_INTERVAL_MS = 360000; // 6 минут
    
    // Общее число фраз phrase_XX.wav
    const uint8_t TOTAL_PHRASES = 1;

    // Имена статических системных треков
    const char* FILE_CAMERA = "/camera.wav";
    const char* FILE_INTRUDER = "/intruder.wav";

    // Спецификаторы форматирования строк
    const char* FMT_ZONE_FILE = "/%02d.wav";
    const char* FMT_PHRASE_FILE = "/phrase_%02d.wav";
};

#endif // ALARM_MANAGER_H
