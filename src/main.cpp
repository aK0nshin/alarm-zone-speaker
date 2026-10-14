#include <Arduino.h>
#include <SPIFFS.h>
#include "AudioPlayer.h"
#include "RadioReceiver.h"

AudioPlayer player(GPIO_NUM_3);
RadioReceiver radio;

enum TrackPriority {
    PRIORITY_LOW,
    PRIORITY_HIGH
};

const char* currentPlayingTrack = "";
TrackPriority currentPriority = PRIORITY_LOW;

const char* queuedTrack = "";
bool hasQueuedTrack = false;

void requestPlay(const char* filePath, TrackPriority priority) {
    if (player.isPlaying() && currentPriority == PRIORITY_HIGH && priority == PRIORITY_LOW) {
        queuedTrack = filePath;
        hasQueuedTrack = true;
        Serial.printf("[Manager] Занято важным треком. Файл %s ушел в очередь.\n", filePath);
        return; 
    }


    Serial.printf("[Manager] Выполнение команды запуска: %s\n", filePath);
    if (!player.play(filePath)) return;

    currentPlayingTrack = filePath;
    currentPriority = priority;
}

void setup() {
    Serial.begin(115200);
    delay(4000); 
    Serial.println("\n=== Спикер запущен ===");

    if (!SPIFFS.begin(true)) {
        Serial.println("[Критическая ошибка] SPIFFS сломан!");
        return;
    }

    player.begin();
    radio.begin();

    Serial.println("[System] Стартовый запуск системы...");
    requestPlay("/territory.wav", PRIORITY_LOW);
}

void loop() {
    RadioCommand command = radio.checkPayload();

    if (command != RadioCommand::NONE) {
        if (command == RadioCommand::ZONE_1_ALARM)  requestPlay("/alarm_zone1.wav", PRIORITY_HIGH);
        if (command == RadioCommand::ZONE_2_SERVICE) requestPlay("/service_zone2.wav", PRIORITY_LOW);
        if (command == RadioCommand::EMERGENCY_STOP) {
            Serial.println("[Manager] Экстренная остановка.");
            player.stop();
            hasQueuedTrack = false;
        }
    }

    if (player.isPlaying()) {
        delay(1);
        return;
    }

    if (!hasQueuedTrack) {
        delay(1);
        return;
    }

    // Действие обработки очереди (линейный код без вложенности)
    Serial.println("[Manager] Плеер освободился. Извлекаем трек из очереди...");
    hasQueuedTrack = false;
    requestPlay(queuedTrack, PRIORITY_LOW);

    delay(1);
}
