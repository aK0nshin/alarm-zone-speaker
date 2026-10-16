#include <Arduino.h>
#include <SPIFFS.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "Mp3Player.h"
#include "RadioReceiver.h"
#include "AlarmManager.h"

Mp3Player player(GPIO_NUM_3);
RadioReceiver radio;
AlarmManager alarmManager(player);
bool audioReady = false;
bool radioReady = false;

void setup() {
    Serial.begin(115200);
    delay(4000);
    Serial.println("\n=== Speaker starting ===");
    Serial.printf("[Startup] Loop stack: %u bytes\n", unsigned(getArduinoLoopTaskStackSize()));
    randomSeed(esp_random());

    if (!SPIFFS.begin(false)) {
        Serial.println("[Startup] SPIFFS mount failed; upload the filesystem image");
        return;
    }
    if (!player.begin() || !alarmManager.begin()) {
        Serial.println("[Startup] Audio initialization failed");
        return;
    }
    radioReady = radio.begin();
    if (!radioReady) Serial.println("[Startup] Radio unavailable; local audio only");
    audioReady = true;
    alarmManager.playStartupTrack();
}

void loop() {
    if (!audioReady) {
        delay(100);
        return;
    }
    if (radioReady) {
        // Bound radio polling so continuous traffic cannot starve audio.
        for (uint8_t i = 0; i < 3; ++i) {
            alarmManager.processCommand(radio.checkPayload());
        }
    }
    bool wasPlaying = player.isPlaying();
    alarmManager.tick();
    static bool stackReported = false;
    if (wasPlaying && !stackReported) {
        Serial.printf("[Audio] Loop stack minimum free: %u bytes\n",
                      unsigned(uxTaskGetStackHighWaterMark(nullptr)));
        stackReported = true;
    }
    delay(1);
}
