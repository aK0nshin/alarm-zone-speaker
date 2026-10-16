#!/bin/sh
set -eu
cd "$(dirname "$0")/.."

encode() {
    ffmpeg -hide_banner -loglevel error -y -i "$1" -map 0:a:0 \
        -ac 1 -ar 24000 -c:a libmp3lame -b:a 64k \
        -map_metadata -1 -id3v2_version 0 -write_id3v1 0 -write_xing 0 "$2"
}

encode data_src/v1/Territory.wav data/territory.mp3
encode data_src/v1/Camera.wav data/camera.mp3
encode data_src/v1/Intruder.wav data/intruder.mp3
encode data_src/i-caught-an-olive.wav data/phrase_01.mp3
for zone in 01 02 03 04 05 06; do
    encode "data_src/v1/$zone.wav" "data/$zone.mp3"
done
