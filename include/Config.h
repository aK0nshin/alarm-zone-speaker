#pragma once

#include <stdint.h>
#include <RadioProtocol.h>

namespace Config {
// Имя группы приёмников и передатчиков: 1..15 байт, регистр важен
constexpr char RADIO_GROUP_NAME[] = "ALPHA";
// Таймаут последовательных тревог с одной камеры, мс; 0 отключает ограничение.
constexpr uint32_t RADIO_CAMERA_COOLDOWN_MS = 3000;

// Мин и макс время между случайными фразами фона, мс;
constexpr uint32_t BG_MIN_INTERVAL_MS = 4000; // 4 секунды
constexpr uint32_t BG_MAX_INTERVAL_MS = 6000; // 6 секунд

// Аппаратные пины SPI для ESP32-C3 Super Mini
constexpr uint8_t RADIO_SCK_PIN = 4;
constexpr uint8_t RADIO_MISO_PIN = 5;
constexpr uint8_t RADIO_MOSI_PIN = 6;
constexpr uint8_t RADIO_SS_PIN = 2;
constexpr uint8_t RADIO_CE_PIN = 10;
constexpr uint8_t RADIO_CSN_PIN = 7;
// GPIO аудиовыхода PDM, подключённого ко входу аналогового усилителя LM4871.
constexpr uint8_t AUDIO_PIN = 3;
}

static_assert(sizeof(Config::RADIO_GROUP_NAME) > 1 &&
              sizeof(Config::RADIO_GROUP_NAME) <= RadioProtocol::GROUP_SIZE,
              "Radio group must contain 1..15 bytes");
static_assert(Config::RADIO_CAMERA_COOLDOWN_MS <= 0x7fffffffUL,
              "Radio cooldown must not exceed 2147483647 ms");
static_assert(Config::BG_MIN_INTERVAL_MS <= Config::BG_MAX_INTERVAL_MS &&
              Config::BG_MAX_INTERVAL_MS < 0x7fffffffUL,
              "Invalid background interval");
