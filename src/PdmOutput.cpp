#include "PdmOutput.h"

bool PdmOutput::begin(uint32_t sampleRate) {
    end();
    _failed = false;
    _sampleRate = sampleRate;
    i2s_config_t config = {};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM);
    config.sample_rate = sampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
    config.dma_buf_count = DMA_COUNT;
    config.dma_buf_len = DMA_SAMPLES;
    config.tx_desc_auto_clear = true;

    esp_err_t error = i2s_driver_install(I2S_NUM_0, &config, 0, nullptr);
    if (error != ESP_OK) {
        Serial.printf("[Audio] I2S install: %s\n", esp_err_to_name(error));
        _failed = true;
        return false;
    }
    _installed = true;

    i2s_pin_config_t pins = {};
    pins.mck_io_num = I2S_PIN_NO_CHANGE;
    pins.bck_io_num = I2S_PIN_NO_CHANGE;
    pins.ws_io_num = I2S_PIN_NO_CHANGE;
    pins.data_out_num = _pin;
    pins.data_in_num = I2S_PIN_NO_CHANGE;
    error = i2s_set_pin(I2S_NUM_0, &pins);
    if (error == ESP_OK) error = i2s_zero_dma_buffer(I2S_NUM_0);
    if (error == ESP_OK) return true;

    Serial.printf("[Audio] I2S setup: %s\n", esp_err_to_name(error));
    _failed = true;
    end();
    return false;
}

void PdmOutput::end() {
    if (_installed) {
        i2s_driver_uninstall(I2S_NUM_0);
        _installed = false;
    }
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

size_t PdmOutput::write(const uint8_t* data, size_t length) {
    if (!_installed || _failed) return 0;
    size_t written = 0;
    esp_err_t error = i2s_write(I2S_NUM_0, data, length, &written, pdMS_TO_TICKS(50));
    if (error != ESP_OK || written != length) {
        _failed = true;
        Serial.printf("[Audio] I2S write failed: %s (%u/%u)\n",
                      esp_err_to_name(error), unsigned(written), unsigned(length));
    }
    return written;
}

uint32_t PdmOutput::drainTimeMs() const {
    // В IDF 4 нет API ожидания завершения передачи; ждём полное кольцо DMA и ещё один буфер.
    return ((DMA_COUNT + 1) * DMA_SAMPLES * 1000UL + _sampleRate - 1) / _sampleRate;
}
