#pragma once

#include "RadioProtocol.h"

#define RADIO_GROUP_NAME "ALPHA"
#define RADIO_CAMERA_COOLDOWN_MS 3000UL

static_assert(sizeof(RADIO_GROUP_NAME) > 1 &&
              sizeof(RADIO_GROUP_NAME) <= RadioProtocol::GROUP_SIZE,
              "Radio group must contain 1..15 bytes");
static_assert(RADIO_CAMERA_COOLDOWN_MS <= 0x7fffffffUL,
              "Radio cooldown must not exceed 2147483647 ms");
