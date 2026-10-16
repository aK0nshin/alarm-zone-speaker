#include <cstdlib>

// The wrapper's default allocator loops forever on OOM; the C decoder handles null.
extern "C" void* helix_malloc(int size) {
    if (size <= 0) return nullptr;
    return std::malloc(static_cast<size_t>(size));
}

extern "C" void helix_free(void* pointer) {
    std::free(pointer);
}
