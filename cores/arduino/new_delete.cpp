#include <stddef.h>
#include <stdint.h>
#include <new>

namespace {
  alignas(max_align_t) static uint8_t g_heap[256];
  static size_t g_heap_used = 0;

  static void* local_alloc(size_t size) {
    if (size == 0) size = 1;

    const size_t align = alignof(max_align_t);
    size = (size + align - 1) & ~(align - 1);

    if (g_heap_used + size > sizeof(g_heap)) {
      return nullptr;
    }

    void* p = &g_heap[g_heap_used];
    g_heap_used += size;
    return p;
  }
}

void* operator new(size_t size) {
  void* p = local_alloc(size);
  if (!p) {
    while (1) {
    }
  }
  return p;
}

void* operator new[](size_t size) {
  return operator new(size);
}

void operator delete(void* ptr) noexcept {
  (void)ptr;
}

void operator delete[](void* ptr) noexcept {
  (void)ptr;
}

void operator delete(void* ptr, size_t) noexcept {
  (void)ptr;
}

void operator delete[](void* ptr, size_t) noexcept {
  (void)ptr;
}