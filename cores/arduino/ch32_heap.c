/*
 * Minimal bump allocator for the CH32 core.
 *
 * The core links with -nostdlib, so this is the only malloc() the sketch and
 * the libraries get. Heap lives between _end (top of BSS) and _heap_end
 * (bottom of the stack).
 *
 * Every block carries an 8-byte header holding its payload size and the
 * previous block, which buys three things a plain bump pointer cannot do:
 *   - realloc() preserves the old contents,
 *   - realloc() grows the newest block in place,
 *   - free() returns memory when blocks are released in LIFO order (the usual
 *     case for String temporaries); a block freed out of order is reclaimed
 *     later, once everything above it has been freed too.
 * It is still not a general-purpose heap: memory released in the middle of the
 * heap is not reused until the blocks above it go away.
 *
 * Payloads are 4-byte aligned, which is what the callers here (String, Wire,
 * SD file names) need. Do not allocate types that require 8-byte alignment.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Symbols provided by the linker script (Link.ld) */
extern char _end;       /* first byte after BSS  */
extern char _heap_end;  /* last byte before stack */

#define HEAP_ALIGN    4u
#define BLK_FREED     0x80000000u  /* flag kept in block_hdr_t.size */

typedef struct block_hdr {
  uint32_t size;   /* usable payload bytes, plus BLK_FREED once released */
  char    *prev;   /* payload of the block allocated before this one */
} block_hdr_t;

static char *heap_base;  /* first header, NULL until the first malloc() */
static char *heap_ptr;   /* next unused byte */
static char *heap_last;  /* payload of the newest live block, NULL if none */

#define HDR(payload)  ((block_hdr_t *)((char *)(payload) - sizeof(block_hdr_t)))
#define BLK_SIZE(h)   ((size_t)((h)->size & ~BLK_FREED))

/* Round up to the payload alignment, refusing sizes that would wrap */
static size_t heap_round(size_t size)
{
  if (size == 0) {
    return HEAP_ALIGN;
  }
  if (size > (size_t) -1 - (HEAP_ALIGN - 1)) {
    return 0;
  }
  return (size + (HEAP_ALIGN - 1)) & ~(size_t)(HEAP_ALIGN - 1);
}

/* True when ptr looks like the payload of a block that is still on the heap */
static int heap_owns(const void *ptr)
{
  const char *p = (const char *)ptr;

  if (!heap_base || !p) {
    return 0;
  }
  if (p < heap_base + sizeof(block_hdr_t) || p > heap_ptr) {
    return 0;
  }
  return (p + BLK_SIZE(HDR(p))) <= heap_ptr;
}

/* Pop every block on top of the heap that has already been freed */
static void heap_trim(void)
{
  while (heap_last && (HDR(heap_last)->size & BLK_FREED)) {
    block_hdr_t *top = HDR(heap_last);
    heap_ptr  = (char *)top;
    heap_last = top->prev;
  }
}

void *malloc(size_t size)
{
  block_hdr_t *h;
  char *payload;

  if (!heap_base) {
    /* Align the very first header; _end is already 4-aligned by the linker */
    heap_base = (char *)(((uintptr_t)&_end + (HEAP_ALIGN - 1)) & ~(uintptr_t)(HEAP_ALIGN - 1));
    heap_ptr  = heap_base;
  }

  size = heap_round(size);
  if (size == 0) {
    return NULL;
  }

  /* Compare as a distance, never as ptr + size, so the check cannot overflow */
  if (heap_ptr >= &_heap_end) {
    return NULL;
  }
  if ((size_t)(&_heap_end - heap_ptr) < size + sizeof(block_hdr_t)) {
    return NULL; /* out of heap */
  }

  h = (block_hdr_t *)heap_ptr;
  h->size = (uint32_t)size;
  h->prev = heap_last;

  payload = heap_ptr + sizeof(block_hdr_t);
  heap_ptr = payload + size;
  heap_last = payload;
  return payload;
}

void free(void *ptr)
{
  if (!heap_owns(ptr)) {
    return; /* NULL, foreign pointer, or a block already reclaimed */
  }

  HDR(ptr)->size |= BLK_FREED;
  heap_trim();
}

void *calloc(size_t nmemb, size_t size)
{
  size_t total;
  void *p;

  if (nmemb && size > (size_t) -1 / nmemb) {
    return NULL; /* nmemb * size would overflow */
  }
  total = nmemb * size;

  p = malloc(total);
  if (p && total) {
    memset(p, 0, total);
  }
  return p;
}

void *realloc(void *ptr, size_t size)
{
  block_hdr_t *h;
  size_t old, want;
  void *np;

  if (!ptr) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  if (!heap_owns(ptr)) {
    return NULL;
  }

  want = heap_round(size);
  if (want == 0) {
    return NULL;
  }

  h = HDR(ptr);
  old = BLK_SIZE(h);
  if (want <= old) {
    return ptr; /* shrinking: keep the block and its recorded size */
  }

  /* The newest block can simply grow into the free space above it */
  if ((char *)ptr == heap_last) {
    size_t extra = want - old;
    if (heap_ptr < &_heap_end && (size_t)(&_heap_end - heap_ptr) >= extra) {
      heap_ptr += extra;
      h->size = (uint32_t)want;
      return ptr;
    }
    return NULL;
  }

  np = malloc(size);
  if (!np) {
    return NULL; /* the old block stays valid, as C requires */
  }
  memcpy(np, ptr, old);
  free(ptr);
  return np;
}
