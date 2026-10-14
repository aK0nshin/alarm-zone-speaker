#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <driver/ledc.h>

class AudioPlayer {
public:
    AudioPlayer(gpio_num_t pin);
    ~AudioPlayer();

    bool begin();
    bool play(const char* filePath); 
    void stop();                     
    bool isPlaying() const;          

private:
    bool parseWavHeader();

    gpio_num_t _audioPin;
    File _audioFile;

    char _trackPathBuffer[64]; 
    
    uint32_t _sampleRate;
    uint32_t _dataSize;
    uint32_t _dataStartOffset;
    
    hw_timer_t* _audioTimer;
    volatile uint32_t _bytesRead;
    volatile bool _isAudioPlaying;

    static AudioPlayer* _instance;
    static void IRAM_ATTR onTimerISR();

    friend void IRAM_ATTR onTimerISR();
};

#endif // AUDIO_PLAYER_H
