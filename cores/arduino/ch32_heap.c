/*
 * Minimal bump allocator for UIAP_HID core.
 * Heap lives between _end (top of BSS) and _heap_end (bottom of stack).
 * free() is a no-op; realloc() and calloc() allocate fresh blocks.
 *
 * Sufficient for small, long-lived allocations such as SD File names.
 */

#include <stddef.h>
#include <string.h>

/* Symbols provided by the linker script (Link.ld) */
extern char _end;       /* first byte after BSS  */
extern char _heap_end;  /* last byte before stack */

static char *heap_ptr = (char *)0;

void *malloc(size_t size)
{
    if (!heap_ptr) heap_ptr = &_end;

    /* Round up to 4-byte alignment */
    size = (size + 3u) & ~3u;
    if (size == 0) size = 4;

    char *ptr = heap_ptr;
    if (ptr + size > &_heap_end) return (void *)0; /* out of heap */

    heap_ptr += size;
    return (void *)ptr;
}

void free(void *ptr)
{
    (void)ptr;
    /* No-op: bump allocator cannot reclaim memory */
}

void *calloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = malloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    /* Simple: allocate a new block (old block is leaked) */
    (void)ptr;
    return malloc(size);
}
