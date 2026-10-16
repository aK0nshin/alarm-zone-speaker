#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

class PdmOutput : public Print {
public:
    explicit PdmOutput(gpio_num_t pin) : _pin(pin) {}
    ~PdmOutput() { end(); }
    PdmOutput(const PdmOutput&) = delete;
    PdmOutput& operator=(const PdmOutput&) = delete;

    bool begin(uint32_t sampleRate);
    void end();
    size_t write(uint8_t value) override { return write(&value, 1); }
    size_t write(const uint8_t* data, size_t length) override;
    uint32_t drainTimeMs() const;

private:
    static constexpr int DMA_COUNT = 4;
    static constexpr int DMA_SAMPLES = 128;
    gpio_num_t _pin;
    bool _installed = false;
    bool _failed = false;
    uint32_t _sampleRate = 8000;
};
