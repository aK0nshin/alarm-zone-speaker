# Используем стабильную Ubuntu
FROM ubuntu:24.04

# Отключаем интерактивные диалоги
ENV DEBIAN_FRONTEND=noninteractive

# Устанавливаем минимальный набор системных утилит (включая git)
RUN apt-get update && apt-get install -y \
    python3 \
    python3-pip \
    git \
    curl \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Скачиваем и устанавливаем uv напрямую через официальный скрипт
COPY --from=ghcr.io/astral-sh/uv:latest /uv /uvx /bin/

# С помощью uv создаем виртуальное окружение и МГНОВЕННО ставим PlatformIO
RUN uv venv /opt/pio-env \
    && . /opt/pio-env/bin/activate \
    && uv pip install -U platformio

# Прописываем путь к PlatformIO в системный PATH контейнера
ENV PATH="/opt/pio-env/bin:$PATH"

# Назначаем рабочую директорию
WORKDIR /workspace

# Открываем консоль при старте
CMD ["/bin/bash"]

# docker run --rm -it -v "${PWD}:/workspace" pio-esp32
# https://espressif.github.io/esptool-js/