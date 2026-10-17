# Общий радиопротокол

Один модуль для камер-передатчиков и спикеров-приёмников. Подключите весь каталог
`RadioProtocol` как локальную библиотеку в проект передатчика. Нужен RF24
(проверено с 1.6.2). Криптографические библиотеки не используются.

## Настройки

В основнои приложении находятся настройки [include/Config.h](../../include/Config.h):


```cpp
namespace Config {
constexpr char RADIO_GROUP_NAME[] = "ALPHA";
}
```

## Логика

- Имя группы (связка камеры + спикеры) работают вместе, то есть спикеры принимают сигналы только от камер из своей группы.
- Подразумевается, что в одной группе не больше 6ти камер. Ограничения на колличество спикеров нет.
- На стороне спикеров есть некоторый кулдаун на принятие одинаковых сигналов, так что можно слать несколько подряд - проиграется одна тревога.
- Сигналы от разных камер складываются в очередь и тревоги проиграются последовательно.
- Команда `RadioCommand::EMERGENCY_STOP` отменяет все тревоги, очищает все очереди.

## Радио и формат версии 3

- Канал 76, скорость 1 Мбит/с, адрес `AZSPK` (5 байт).
- Фиксированная нагрузка 20 байт, CRC 16 бит.
- Dynamic payloads и auto-ACK выключены для групповой передачи.
- Байты 0..1: ASCII `AZ`; байт 2: версия 3; байт 3: команда.
- Байты 4..19: имя группы, дополненное нулями до 16 байт.
- Команды: числовые 1..6 для зон и 99 для остановки. ASCII-команд нет.

Все группы используют общий радиоадрес; приёмник сравнивает полное имя в пакете.
Пины, питание и мощность радиомодуля задаёт конкретное устройство.
Вызывайте `configureReceiver` или `configureTransmitter` после успешного `radio.begin()`.

## Пример отправки

```cpp
#include <RadioProtocolRF24.h>
#include "Config.h"
#include <esp_system.h>

constexpr RadioCommand CAMERA_ZONE = RadioCommand::ZONE_1_ALARM;
bool radioReady = false;

void beginRadio() {
    radioReady = false;
    if (!radio.begin()) return;
    radio.setPALevel(RF24_PA_LOW);
    radioReady = RadioProtocol::configureTransmitter(radio);
}

void sendCommand(RadioCommand command) {
    if (!radioReady) return;
    RadioProtocol::Packet packet;
    if (!RadioProtocol::encode(command, Config::RADIO_GROUP_NAME, packet)) return;
    for (unsigned copy = 0; copy < 3; ++copy) {
        if (copy) delay(20 + esp_random() % 41);
        radio.write(packet.data(), packet.size());
    }
}
```

Пример делает три отправки со случайными паузами; при положительном интервале,
превышающем длительность этих повторов, спикер пропустит повторные тревоги.
В рабочем приложении камеры можно заменить delay неблокирующим планировщиком.
Не вызывайте отправку из ISR.
