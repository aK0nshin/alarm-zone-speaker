#include "AudioPlayer.h"

#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_RESOLUTION     LEDC_TIMER_8_BIT
#define LEDC_BASE_FREQ      312500 

AudioPlayer* AudioPlayer::_instance = nullptr;

void IRAM_ATTR AudioPlayer::onTimerISR() {
    // Ранний выход, если плеер выключен или инстанса нет
    if (!_instance || !_instance->_isAudioPlaying) return;

    // Ранний выход, если файл кончился или недоступен
    if (!_instance->_audioFile || !_instance->_audioFile.available() || _instance->_bytesRead >= _instance->_dataSize) {
        _instance->_isAudioPlaying = false;
        return;
    }

    // Полезная работа прерывания (без вложенных if)
    uint8_t sample = _instance->_audioFile.read();
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL, sample);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL);
    _instance->_bytesRead++;
}

AudioPlayer::AudioPlayer(gpio_num_t pin) {
    _audioPin = pin;
    _audioTimer = NULL;
    _bytesRead = 0;
    _isAudioPlaying = false;
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
    stop(); // Прерываем старый файл перед запуском нового
    
    _audioFile = SPIFFS.open(filePath, "r");
    if (!_audioFile) return false; // Ранний выход: файл не найден

    if (!parseWavHeader()) {
        _audioFile.close();
        return false; // Ранний выход: заголовок битый
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
        timerAttachInterrupt(_audioTimer, &AudioPlayer::onTimerISR, true);
    }

    timerAlarmWrite(_audioTimer, 1000000 / _sampleRate, true);
    _isAudioPlaying = true;
    timerAlarmEnable(_audioTimer);
    return true;
}

void AudioPlayer::stop() {
    _isAudioPlaying = false;
    
    if (_audioTimer) timerAlarmDisable(_audioTimer);
    if (_audioFile) _audioFile.close();

    gpio_reset_pin(_audioPin);
    pinMode(_audioPin, OUTPUT);
    digitalWrite(_audioPin, LOW);
}

bool AudioPlayer::isPlaying() const {
    return _isAudioPlaying;
}

bool AudioPlayer::parseWavHeader() {
    if (_audioFile.size() < 44) return false; // Ранний выход: файл слишком мелкий
    
    uint8_t header[44];
    _audioFile.read(header, 44);
    
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) return false; // Ранний выход: не WAV
    
    _sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    uint16_t bitsPerSample = header[34] | (header[35] << 8);
    if (bitsPerSample != 8) return false; // Ранний выход: не 8-бит
    
    uint32_t offset = 12;
    while (offset < _audioFile.size() - 8) {
        _audioFile.seek(offset);
        uint8_t chunkId[4];
        _audioFile.read(chunkId, 4);
        uint32_t chunkSize = _audioFile.read() | (_audioFile.read() << 8) | (_audioFile.read() << 16) | (_audioFile.read() << 24);
        
        if (memcmp(chunkId, "data", 4) == 0) {
            _dataSize = chunkSize;
            _dataStartOffset = offset + 8;
            return true; // Ранний выход при успехе: нашли аудиопоток
        }
        offset += 8 + chunkSize;
    }
    return false; // Ранний выход: чанк data не найден
}
