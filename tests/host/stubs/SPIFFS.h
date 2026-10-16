#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>
#include <fstream>
#include <map>
#include <string>
#include <cstring>

inline std::map<std::string, std::vector<uint8_t>> testFiles;
namespace fs {
class File {
public:
    File() = default;
    explicit File(std::vector<uint8_t> data)
        : _data(std::make_shared<std::vector<uint8_t>>(std::move(data))) {}
    explicit operator bool() const { return bool(_data); }
    bool isDirectory() const { return false; }
    size_t size() const { return _data ? _data->size() : 0; }
    size_t position() const { return _position; }
    bool seek(uint32_t position) {
        if (!_data || position > size()) return false;
        _position = position;
        return true;
    }
    size_t read(uint8_t* output, size_t count) {
        if (!_data) return 0;
        count = std::min(count, size() - _position);
        std::memcpy(output, _data->data() + _position, count);
        _position += count;
        return count;
    }
    void close() { _data.reset(); }
private:
    std::shared_ptr<std::vector<uint8_t>> _data;
    size_t _position = 0;
};
}
struct TestSpiffs {
    fs::File open(const char* path, const char*) {
        auto found = testFiles.find(path);
        if (found != testFiles.end()) return fs::File(found->second);
        std::ifstream input(std::string("data") + path, std::ios::binary);
        if (!input) return {};
        return fs::File(std::vector<uint8_t>((std::istreambuf_iterator<char>(input)), {}));
    }
};
inline TestSpiffs SPIFFS;
