#include "AlarmManager.h"

AlarmManager::AlarmManager(AudioPlayer& player) 
    : _player(player), _currentPriority(AudioPriority::NONE), 
      _pipeHead(0), _pipeTail(0) {}

void AlarmManager::begin() {
    resetBackgroundTimer();
}

void AlarmManager::playStartupTrack() {
    Serial.println("[Manager] Загрузка стартового трека системы безопасности...");
    _currentPriority = AudioPriority::ALARM;
    _lastBackgroundPlayTime = millis();
    
    addTrackToPipeline(FILE_STARTUP);
}

void AlarmManager::processCommand(RadioCommand cmd) {
    if (cmd == RadioCommand::NONE) return; // Ранний выход

    if (static_cast<int>(cmd) == EMERGENCY_STOP_CODE) {
        Serial.println("[Manager] Экстренный останов. Очистка конвейера.");
        clearPipeline();
        return;
    }

    int zone = static_cast<int>(cmd);
    if (zone < MIN_ZONE_NUMBER || zone > MAX_ZONE_NUMBER) return;

    if (_currentPriority == AudioPriority::NON_IMPORTANT) {
        Serial.println("[Manager] Тревога вытесняет фоновую фразу. Перезапуск конвейера.");
        clearPipeline();
    }

    _currentPriority = AudioPriority::ALARM;
    _lastBackgroundPlayTime = millis(); 

    char zoneFile[8];
    snprintf(zoneFile, sizeof(zoneFile), FMT_ZONE_FILE, zone);

    Serial.printf("[Manager] Загрузка в конвейер пакета тревоги для Зоны %d\n", zone);
    addTrackToPipeline(FILE_CAMERA);
    addTrackToPipeline(zoneFile);
    addTrackToPipeline(FILE_INTRUDER);
}

void AlarmManager::tick() {
    if (_player.isPlaying()) return; 

    if (_pipeHead != _pipeTail) {
        String nextTrack = _pipelineQueue[_pipeHead];
        _pipeHead = (_pipeHead + 1) % PIPELINE_SIZE;

        if (!_player.play(nextTrack.c_str())) {
            Serial.printf("[ОШИБКА КОНВЕЙЕРА] Пропуск битого файла: %s\n", nextTrack.c_str());
        } else {
            Serial.printf("[Pipeline] Запущено исполнение: %s\n", nextTrack.c_str());
        }
        return;
    }

    if (_currentPriority == AudioPriority::ALARM) {
        _currentPriority = AudioPriority::NONE;
        resetBackgroundTimer(); 
    }

    if (millis() - _lastBackgroundPlayTime >= _nextBackgroundInterval) {
        if (_currentPriority != AudioPriority::NONE || _pipeHead != _pipeTail || _player.isPlaying()) {
            Serial.println("[Manager] Сработал таймер фразы, но система занята. Пропуск.");
            resetBackgroundTimer(); 
            return;
        }
        playBackgroundPhrase();
    }
}

void AlarmManager::addTrackToPipeline(const char* filePath) {
    uint8_t nextTail = (_pipeTail + 1) % PIPELINE_SIZE;
    
    if (nextTail == _pipeHead) {
        Serial.printf("[Pipeline] КРИТИЧЕСКАЯ ОШИБКА: Конвейер переполнен! Файл %s отброшен.\n", filePath);
        return;
    }

    _pipelineQueue[_pipeTail] = filePath;
    _pipeTail = nextTail;
}

void AlarmManager::playBackgroundPhrase() {
    _currentPriority = AudioPriority::NON_IMPORTANT;
    
    int randomId = random(1, TOTAL_PHRASES + 1); 
    
    char phraseFile[15];
    snprintf(phraseFile, sizeof(phraseFile), FMT_PHRASE_FILE, randomId);

    Serial.printf("[Manager] Загрузка в конвейер фоновой фразы: %s\n", phraseFile);
    addTrackToPipeline(phraseFile);
    
    resetBackgroundTimer();
}

void AlarmManager::resetBackgroundTimer() {
    _lastBackgroundPlayTime = millis();
    _nextBackgroundInterval = random(BG_MIN_INTERVAL_MS, BG_MAX_INTERVAL_MS + 1);
}

void AlarmManager::clearPipeline() {
    _player.stop();
    _pipeHead = 0;
    _pipeTail = 0;
    _currentPriority = AudioPriority::NONE;
    resetBackgroundTimer();
}
