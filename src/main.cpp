#include <Arduino.h>
#include <SPIFFS.h>
#include "AudioPlayer.h"
#include "RadioReceiver.h"
#include "AlarmManager.h"

AudioPlayer player(GPIO_NUM_3);
RadioReceiver radio;
AlarmManager alarmManager(player);

void setup() {
    Serial.begin(115200);
    delay(4000); 
    Serial.println("\n=== ОХРАННЫЙ КОМПЛЕКС ЗАПУЩЕН ===");

    if (!SPIFFS.begin(true)) {
        Serial.println("[Критическая ошибка] SPIFFS поврежден!");
        return;
    }

    player.begin();
    radio.begin();
    alarmManager.begin();

    randomSeed(analogRead(0)); 
}

void loop() {
    RadioCommand command = radio.checkPayload();

    alarmManager.processCommand(command);

    alarmManager.tick();

    delay(1);
}
