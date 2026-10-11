#include <Arduino.h>
#include "SPIFFS.h"

// Константы аудио
const int SAMPLE_RATE = 16000;
const int LEDC_CHANNEL = 0;

const int TOTAL_FILES = 4;
const char* audioFiles[TOTAL_FILES] = {
    "/sound.wav",
    "/sound_loud.wav",
    "/sound_boost.wav",
    "/sound_ultra.wav"
};
int currentFileIndex = 0;

File audioFile;
volatile bool isPlaying = false;

const size_t RING_BUFFER_SIZE = 4096;
uint8_t ringBuffer[RING_BUFFER_SIZE];
volatile size_t head = 0;
volatile size_t tail = 0;

hw_timer_t* audioTimer = nullptr;

unsigned long pauseStartTime = 0;
bool inPause = false;
const unsigned long PAUSE_DURATION = 2000;

size_t getBufferFreeSpace() {
    size_t h = head;
    size_t t = tail;
    if (h >= t) return RING_BUFFER_SIZE - (h - t) - 1;
    return t - h - 1;
}

void refillBuffer() {
    if (!audioFile || !audioFile.available()) return;

    size_t freeSpace = getBufferFreeSpace();
    const size_t CHUNK_SIZE = 256; 
    uint8_t chunkBuffer[CHUNK_SIZE];

    while (freeSpace >= CHUNK_SIZE && audioFile.available()) {
        size_t bytesRead = audioFile.read(chunkBuffer, CHUNK_SIZE);
        for (size_t i = 0; i < bytesRead; i++) {
            ringBuffer[head] = chunkBuffer[i];
            head = (head + 1) % RING_BUFFER_SIZE;
        }
        freeSpace -= bytesRead;
    }
}

void IRAM_ATTR onTimer() {
    if (!isPlaying) return;

    if (head != tail) {
        ledcWrite(LEDC_CHANNEL, ringBuffer[tail]);
        tail = (tail + 1) % RING_BUFFER_SIZE;
    }
}

void stopPlayback() {
    isPlaying = false;
    if (audioFile) {
        audioFile.close();
    }
    ledcWrite(LEDC_CHANNEL, 128); // Удерживаем тишину
    head = 0;
    tail = 0;
}

// Новая функция: Готовит файл ЗАРАНЕЕ, пока идет пауза
void prepareNextFile() {
    if (audioFile) audioFile.close();
    
    head = 0;
    tail = 0;
    for (size_t i = 0; i < RING_BUFFER_SIZE; i++) {
        ringBuffer[i] = 128;
    }

    const char* fileName = audioFiles[currentFileIndex];
    Serial.printf("Физическое открытие файла (в фоне): %s\r\n", fileName);
    Serial.flush();

    audioFile = SPIFFS.open(fileName, "r");
    if (!audioFile) {
        Serial.printf("Ошибка открытия %s!\r\n", fileName);
        return;
    }

    if (audioFile.size() > 44) {
        uint8_t dummyHeader[44];
        audioFile.read(dummyHeader, 44);
    }

    // Закачиваем первые блоки в буфер ЗАРАНЕЕ. 
    // Теперь всплеск нагрузки на шину Flash происходит во время паузы!
    refillBuffer(); 
}

void startPlayback() {
    Serial.printf("Мягкий старт звука (Ступень %d/%d)\r\n", currentFileIndex + 1, TOTAL_FILES);
    Serial.flush();
    
    // Перед стартом принудительно выставляем указатели буфера, 
    // чтобы таймер начал читать строго с первого заполненного байта звука
    tail = 0; 
    
    // Включаем тумблер. Буфер полон, файл открыт, прерывание теперь работает идеально
    isPlaying = true; 
}


void setup() {
    Serial.begin(115200);
    delay(4000); // Безопасная пауза из вашего README

    if (!SPIFFS.begin(true)) {
        Serial.println("Ошибка SPIFFS!");
        while (1);
    }

    // Настройка аппаратного ШИМ для ESP32-C3 (78.1 кГц, 8 бит)
    ledcSetup(LEDC_CHANNEL, 78125, 8);
    ledcAttachPin(3, LEDC_CHANNEL);
    ledcWrite(LEDC_CHANNEL, 128); // Удерживаем тишину на старте

    // ИСПРАВЛЕННАЯ НАСТРОЙКА ТАЙМЕРА ПОД ЯДРО 2.Х
    // Первый параметр: 0 (номер таймера)
    // Второй параметр: 80 (делитель частоты, 80 МГц / 80 = 1 МГц, 1 тик = 1 мкс)
    // Третий параметр: true (считать вверх)
    audioTimer = timerBegin(0, 80, true);
    
    // ФИКС ТРЫКА: Третий параметр ставим в FALSE! 
    // Это принудительно переключает прерывание в режим LEVEL (по уровню),
    // что аппаратно требуется для чипа ESP32-C3 и убирает хаотичные повторные вызовы!
    timerAttachInterrupt(audioTimer, &onTimer, false);
    
    // Задаем интервал вызова. Для 16000 Гц: 1000000 / 16000 = 62.5 мкс (ставим 62)
    // Третий параметр: true (автоперезапуск таймера)
    timerAlarmWrite(audioTimer, 62, true);
    
    // Активируем таймер
    timerAlarmEnable(audioTimer);

    // Готовим первый файл в паузе и запускаем систему
    prepareNextFile();
    startPlayback();
}

void loop() {
    if (isPlaying) {
        if (head == tail && !audioFile.available()) {
            Serial.println("Воспроизведение завершено. Уходим на паузу.\r\n");
            stopPlayback();

            // Переключаем индекс на будущее
            currentFileIndex = (currentFileIndex + 1) % TOTAL_FILES;
            
            inPause = true;
            pauseStartTime = millis();
        }
        refillBuffer();
    } 
    else if (inPause) {
        // ТРЮК: Когда прошла ровно половина паузы (1 секунда), 
        // тихо открываем следующий файл на флешке в фоновом режиме!
        static bool filePrepared = false;
        
        if (!filePrepared && (millis() - pauseStartTime >= (PAUSE_DURATION / 2))) {
            prepareNextFile();
            filePrepared = true;
        }

        // Когда пауза полностью закончилась — просто мгновенно включаем тумблер звука
        if (millis() - pauseStartTime >= PAUSE_DURATION) {
            inPause = false;
            filePrepared = false; // Сброс флага для следующего цикла
            startPlayback();
        }
    }
}
