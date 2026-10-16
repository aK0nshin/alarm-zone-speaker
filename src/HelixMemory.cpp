#include <cstdlib>

// Штатный аллокатор обёртки зацикливается при нехватке памяти; C-декодер обрабатывает null.
extern "C" void* helix_malloc(int size) {
    if (size <= 0) return nullptr;
    return std::malloc(static_cast<size_t>(size));
}

extern "C" void helix_free(void* pointer) {
    std::free(pointer);
}
