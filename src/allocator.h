//
// Created by panda on 10/6/19.
//

#ifndef UNTITLED_ALLOCATOR_H
#define UNTITLED_ALLOCATOR_H

/**
 * Machine word size. Depending on the architecture,
 * can be 4 or 8 bytes.
 */
#include <unistd.h>   // for sbrk
#include <stdbool.h>
#include <stdlib.h>

typedef intptr_t word_t;

typedef struct Block Block;
/**
 * Allocated block of memory. Contains the object header structure,
 * and the actual payload pointer.
 */
struct Block {

  // -------------------------------------
  // 1. Object header

  /**
   * Block size.
   */
  size_t size;

  /**
   * Whether this block is currently used.
   */
  bool used;

  /**
   * Next block in the list.
   */
  Block *next;

  Block *next_free;
  Block *prev_free;
  // -------------------------------------
  // 2. User data

  /**
   * Payload pointer.
   */
  word_t data[1];

};


static Block *segregated_starts[] = {
    NULL,   //   8
    NULL,   //  16
    NULL,   //  32
    NULL,   //  64
    NULL,   // 128
    NULL,   // any > 128
    NULL,
    NULL,
    NULL,
};

static Block *segregated_tops[] = {
    NULL,   //   8
    NULL,   //  16
    NULL,   //  32
    NULL,   //  64
    NULL,   // 128
    NULL,   // any > 128
    NULL,
    NULL,
    NULL,
};

static Block *segregated_tops_free[] = {
    NULL,   //   8
    NULL,   //  16
    NULL,   //  32
    NULL,   //  64
    NULL,   // 128
    NULL,   // any > 128
    NULL,
    NULL,
    NULL,
};

static Block *segregated_starts_free[] = {
    NULL,   //   8
    NULL,   //  16
    NULL,   //  32
    NULL,   //  64
    NULL,   // 128
    NULL,   // any > 128
    NULL,
    NULL,
    NULL,
};

static size_t free_lists_size[] = {
    0,   //   8
    0,   //  16
    0,   //  32
    0,   //  64
    0,   // 128
    0,   // any > 128
    0,
    0,
    0,
};

static Block *heap_start = NULL;
static Block *top = NULL;
static Block *heap_start_free = NULL;
static Block *top_free = NULL;
static size_t free_list_size = 0;

struct mem {
  void **(*alloc)(size_t);

  void (*free)(void *);
};

extern const struct mem memory;


/**
 * Mode for searching a free block.
 */
typedef enum {
  first_fit,
  next_fit,
  best_fit,
  free_list,
  segregated_list,
} search_mode_enum;

static const size_t total_buckets = 9;

static const size_t max_size = 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2 * 2;

/**
 * Current search mode.
 */
static int search_mode = first_fit;

/**
 * Previously found block. Updated in `next_fit`.
 */
static Block *search_start = NULL;

/**
 * Tries to find a block that fits.
 */
static Block *(*find_block)(size_t size);

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
static Block *(*split)(Block *block, size_t size);

/**
 * Coalesces two adjacent blocks.
 */
static Block *(*coalesce)(Block *block);

/**
 * Coalesces two adjacent blocks.
 */
static void (*alloc_internal)(Block *block);

/**
 * Allocates a block from the list, splitting if needed.
 */
Block *list_allocate(Block *block, size_t size);

Block *first_fit_search(size_t size);

/**
 * Next-fit algorithm.
 *
 * Returns the next free block which fits the size.
 * Updates the `search_start` of success.
 */
Block *next_fit_search(size_t size);

Block *best_fit_search(size_t size);

Block *free_list_search(size_t size);

/**
 * Segregated fit algorithm.
 */
Block *segregated_fit_search(size_t size);

/**
 * Internal free call.
 */
static void (*free_f)(word_t *data);

/**
* Allocates a block of memory of (at least) `size` bytes.
*/
void *alloc(size_t size);

Block *get_header(word_t *data);

Block *request_mem_from_OS(size_t size);

void free_mem(void *data);

/**
 * Reset the heap to the original position.
 */void reset_heap();

/**
 * Initializes the heap, and the search mode.
 */
void init(search_mode_enum mode);

Block **get_segregated_starts();

Block **get_segregated_starts_free();

const Block *get_search_start();

size_t get_free_list_size();

/**
* Aligns the size by the machine word.
*/
static inline size_t align(size_t n) {
  return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

/**
 * Gets the bucket number from segregatedLists
 * based on the size.
 */
static inline int get_bucket(size_t size) {
  const size_t nbucket = size / sizeof(word_t) - 1;
  return nbucket > total_buckets ? total_buckets : nbucket;
}

/**
 * Returns total allocation size, reserving in addition the space for
 * the Block structure (object header + first data word).
 *
 * Since the `word_t data[1]` already allocates one word inside the Block
 * structure, we decrease it from the size request: if a user allocates
 * only one word, it's fully in the Block struct.
 */
static inline size_t calc_alloc_size(size_t size) {
  return size + sizeof(Block) - sizeof(word_t[1]);
}


/**
 * Whether this block can be split.
 */
static inline bool can_split(Block *block, size_t size) {
  return block->size - size >= sizeof(Block);
}

/**
 * Whether we should merge this block.
 */
static inline bool can_coalesce(Block *block) {
  return block->next && !block->next->used;
}

#endif //UNTITLED_ALLOCATOR_H
