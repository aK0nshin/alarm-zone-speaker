#pragma once
#include <deque>
#include <vector>
#include <cstring>
struct StaticQueue_t {
    size_t capacity = 0;
    size_t itemSize = 0;
    std::deque<std::vector<uint8_t>> items;
};
using QueueHandle_t = StaticQueue_t*;
inline QueueHandle_t xQueueCreateStatic(size_t capacity, size_t itemSize, uint8_t*, StaticQueue_t* queue) {
    queue->capacity = capacity;
    queue->itemSize = itemSize;
    return queue;
}
inline int xQueueSend(QueueHandle_t queue, const void* item, int) {
    if (queue->items.size() == queue->capacity) return 0;
    const auto* bytes = static_cast<const uint8_t*>(item);
    queue->items.emplace_back(bytes, bytes + queue->itemSize);
    return pdPASS;
}
inline int xQueueReceive(QueueHandle_t queue, void* item, int) {
    if (queue->items.empty()) return 0;
    std::memcpy(item, queue->items.front().data(), queue->itemSize);
    queue->items.pop_front();
    return pdPASS;
}
inline void xQueueReset(QueueHandle_t queue) { queue->items.clear(); }
inline void vQueueDelete(QueueHandle_t queue) { queue->items.clear(); }
