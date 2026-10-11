#!/bin/bash
# Останавливать скрипт при любой ошибке
set -e

echo "=== 1/3 Очистка старого кэша ==="
pio run --target clean

echo "=== 2/3 Сборка файловой системы LittleFS ==="
pio run --target buildfs

echo "=== 3/3 Компиляция кода для ESP32-C3 ==="
pio run

echo "=== СБОРКА УСПЕШНО ЗАВЕРШЕНА! ==="
