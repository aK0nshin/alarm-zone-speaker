#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <algorithm>
using gpio_num_t = int;
constexpr gpio_num_t GPIO_NUM_3 = 3;
inline uint32_t testMillis = 0;
inline uint32_t millis() { return testMillis; }
inline long random(long low, long) { return low; }
struct TestSerial {
    void println(const char*) {}
    template<typename... Args> void printf(const char*, Args...) {}
};
inline TestSerial Serial;
class Print {
public:
    virtual ~Print() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t*, size_t) = 0;
};
