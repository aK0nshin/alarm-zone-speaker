#!/bin/sh
set -eu
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Itests/host/stubs -Iinclude -I.pio/libdeps/esp32-c3/dr_libs \
    tests/host/test_firmware.cpp src/AlarmManager.cpp src/WavPlayer.cpp \
    -o /tmp/alarm-zone-speaker-tests
/tmp/alarm-zone-speaker-tests
