#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

const int AUDIO_PIN = 3; 
const int LEDC_CHANNEL = 0;

File audioFile;

// Настройки точного ШИМ-таймера
const uint32_t SAMPLE_INTERVAL_MICROS = 45; 
uint32_t lastSampleTime = 0;

// Настройки циклического повтора
bool isWaitingForRestart = false;
uint32_t waitStartTime = 0;
const uint32_t PAUSE_BETWEEN_LOOPS_MS = 2000; // 2 секунды задержки

// Функция для безопасного открытия файла
bool startPlayback() {
    audioFile = SPIFFS.open("/sound.wav", "r");
    if (!audioFile) {
        Serial.println("Ошибка: Не удалось открыть /sound.wav");
        return false;
    }
    // Перепривязываем ШИМ к ноге при старте нового цикла
    ledcAttachPin(AUDIO_PIN, LEDC_CHANNEL);
    Serial.println("-> Запуск воспроизведения трека...");
    return true;
}

void setup() {
    Serial.begin(115200);
    
    // Пауза для инициализации USB-CDC в Windows
    delay(4000); 

    Serial.println("--- ТЕРМИНАЛ ПОДКЛЮЧЕН. НАЧАЛО ДЕБАГА ---");

    if(!SPIFFS.begin(true)){
        Serial.println("Ошибка монтирования SPIFFS!");
        return;
    }

    ledcSetup(LEDC_CHANNEL, 22050, 8);
    
    // Запускаем первый цикл воспроизведения
    startPlayback();
}

void loop() {
    // Состояние 1: Файл открыт и воспроизводится
    if (audioFile && audioFile.available()) {
        if (micros() - lastSampleTime >= SAMPLE_INTERVAL_MICROS) {
            lastSampleTime = micros();
            uint8_t audioSample = audioFile.read();
            ledcWrite(LEDC_CHANNEL, audioSample);
        }
    } 
    // Состояние 2: Файл только что закончился, инициализируем паузу
    else if (audioFile && !isWaitingForRestart) {
        audioFile.close();
        ledcDetachPin(AUDIO_PIN); // Отключаем ШИМ на время паузы (убирает гул)
        
        isWaitingForRestart = true;
        waitStartTime = millis(); // Запоминаем время начала паузы
        Serial.println("-> Трек завершён. Ждём 2 секунды...");
    } 
    // Состояние 3: Крутимся в неблокирующем ожидании 2 секунд
    else if (isWaitingForRestart) {
        if (millis() - waitStartTime >= PAUSE_BETWEEN_LOOPS_MS) {
            isWaitingForRestart = false; // Сбрасываем флаг ожидания
            Serial.println("-> Пауза прошла!");
            startPlayback(); // Запускаем трек по новой
        }
    }
}
