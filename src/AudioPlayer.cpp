#include "AudioPlayer.h"

#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_RESOLUTION     LEDC_TIMER_8_BIT
#define LEDC_BASE_FREQ      312500 

AudioPlayer* AudioPlayer::_instance = nullptr;

void IRAM_ATTR AudioPlayer::onTimerISR() {
    if (!_instance || !_instance->_isAudioPlaying) return;

    if (!_instance->_audioFile || !_instance->_audioFile.available() || _instance->_bytesRead >= _instance->_dataSize) {
        _instance->_isAudioPlaying = false;
        return;
    }

    uint8_t sample = _instance->_audioFile.read();
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, sample);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    _instance->_bytesRead++;
}

AudioPlayer::AudioPlayer(gpio_num_t pin) {
    _audioPin = pin;
    _audioTimer = nullptr; // Явно пишем nullptr вместо NULL для строгости C++
    _bytesRead = 0;
    _isAudioPlaying = false;
    memset(_trackPathBuffer, 0, sizeof(_trackPathBuffer));
    _instance = this;
}

AudioPlayer::~AudioPlayer() {
    stop();
}

bool AudioPlayer::begin() {
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_RESOLUTION,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_BASE_FREQ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    return (ledc_timer_config(&ledc_timer) == ESP_OK);
}

bool AudioPlayer::play(const char* filePath) {
    if (filePath == nullptr) return false;
    strncpy(_trackPathBuffer, filePath, sizeof(_trackPathBuffer) - 1);
    _trackPathBuffer[sizeof(_trackPathBuffer) - 1] = '\0'; // Гарантированный нуль-терминатор

    Serial.printf("[Debug-Player] Старт воспроизведения безопасного пути: %s\n", _trackPathBuffer); 
    Serial.flush();

    stop(); // Глушим старые прерывания
    
    // Открываем файл из собственного буфера, который никуда не исчезнет из памяти
    _audioFile = SPIFFS.open(_trackPathBuffer, "r");
    if (!_audioFile) {
        Serial.println("[Debug-Player] ОШИБКА: Файл не найден на SPIFFS!"); 
        Serial.flush();
        return false;
    }

    if (!parseWavHeader()) {
        Serial.println("[Debug-Player] ОШИБКА: Неверный формат заголовка!"); 
        Serial.flush();
        _audioFile.close();
        return false;
    }

    _audioFile.seek(_dataStartOffset);
    _bytesRead = 0;

    ledc_channel_config_t ledc_channel = {
        .gpio_num       = _audioPin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);

    if (_audioTimer == NULL) {
        _audioTimer = timerBegin(0, 80, true);
        timerAttachInterrupt(_audioTimer, &AudioPlayer::onTimerISR, false);
    }

    uint64_t alarmIntervalMicros = 1000000 / _sampleRate;
    timerAlarmWrite(_audioTimer, alarmIntervalMicros, true);
    
    _isAudioPlaying = true;
    timerAlarmEnable(_audioTimer);

    Serial.println("[Debug-Player] Аппаратный таймер запущен успешно."); 
    Serial.flush();
    return true;
}

void AudioPlayer::stop() {
    _isAudioPlaying = false;
    Serial.println("[Debug-Stop] Вошли в stop()"); Serial.flush();

    if (_audioTimer != NULL) {
        Serial.printf("[Debug-Stop] Таймер инициализирован (адрес: %p). Отключаем... ", _audioTimer); Serial.flush();
        timerAlarmDisable(_audioTimer);
        Serial.println("Успешно."); Serial.flush();
    } else {
        Serial.println("[Debug-Stop] Таймер равен NULL, пропуск отключения."); Serial.flush();
    }
    
    Serial.print("[Debug-Stop] Закрытие файла... "); Serial.flush();
    if (_audioFile) {
        _audioFile.close();
    }
    Serial.println("Успешно."); Serial.flush();

    Serial.print("[Debug-Stop] Сброс ШИМ (LEDC)... "); Serial.flush();
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    Serial.println("Успешно."); Serial.flush();

    Serial.print("[Debug-Stop] Физический сброс пина... "); Serial.flush();
    gpio_reset_pin(_audioPin);
    pinMode(_audioPin, OUTPUT);
    digitalWrite(_audioPin, LOW);
    Serial.println("Успешно. Выходим из stop()"); Serial.flush();
}

bool AudioPlayer::isPlaying() const {
    return _isAudioPlaying;
}

bool AudioPlayer::parseWavHeader() {
    if (_audioFile.size() < 44) return false;
    
    // ЖЕСТКИЙ ФИКС: Объявляем честный массив на 44 байта, чтобы не затирать стек ядра!
    uint8_t header[44]; 
    _audioFile.read(header, 44);
    
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) return false;
    
    _sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    
    uint16_t bitsPerSample = header[34] | (header[35] << 8);
    if (bitsPerSample != 8) return false;
    
    uint32_t offset = 12;
    while (offset < _audioFile.size() - 8) {
        _audioFile.seek(offset);
        uint8_t chunkId[4];
        _audioFile.read(chunkId, 4);
        uint32_t chunkSize = _audioFile.read() | (_audioFile.read() << 8) | (_audioFile.read() << 16) | (_audioFile.read() << 24);
        
        if (memcmp(chunkId, "data", 4) == 0) {
            _dataSize = chunkSize;
            _dataStartOffset = offset + 8;
            return true;
        }
        offset += 8 + chunkSize;
    }
    return false;
}
