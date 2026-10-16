#!/bin/sh
set -eu
build=$(mktemp -d)
trap 'rm -rf "$build"' EXIT
g++ -std=c++11 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -Itests/host/radio_stubs -Ilib/RadioProtocol/src -Iinclude \
    tests/host/test_radio_protocol.cpp lib/RadioProtocol/src/RadioProtocol.cpp \
    -o "$build/radio-tests"
"$build/radio-tests"
helix=.pio/libdeps/esp32-c3/libhelix/src
for source in "$helix"/libhelix-mp3/*.c; do
    gcc -O2 -fwrapv -fsanitize=address -I"$helix" -c "$source" -o "$build/$(basename "$source").o"
done
for name in 01 02 03 04 05 06 camera intruder territory phrase_01; do
    ffmpeg -hide_banner -loglevel error -i "data/$name.mp3" -f s16le "$build/$name.pcm"
done
g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Itests/host/stubs -Iinclude -Ilib/RadioProtocol/src \
    -I"$helix" \
    tests/host/test_firmware.cpp src/AlarmManager.cpp \
    src/Mp3Player.cpp src/HelixMemory.cpp "$build"/*.o \
    -o "$build/tests"
"$build/tests" "$build"
