//
// Created by panda on 10/6/19.
//

#ifndef UNTITLED_ALLOCATOR_H
#define UNTITLED_ALLOCATOR_H

/**
 * Machine word size. Depending on the architecture,
 * can be 4 or 8 bytes.
 */
#include <cstdint>
#include <unistd.h>   // for sbrk
#include <utility>

using word_t = intptr_t;

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

static Block *heap_start = nullptr;
static auto top = heap_start;

static Block *heap_start_free = nullptr;
static auto top_free = heap_start_free;
static size_t heap_size = 0;
static size_t free_list_size = 0;
/**
 * Previously found block. Updated in `next_fit`.
 */
static auto search_start = heap_start;

/**
* Allocates a block of memory of (at least) `size` bytes.
*/
word_t *alloc(size_t size);

/**
* Aligns the size by the machine word.
*/
inline size_t align(size_t n);

/**
 * Returns total allocation size, reserving in addition the space for
 * the Block structure (object header + first data word).
 *
 * Since the `word_t data[1]` already allocates one word inside the Block
 * structure, we decrease it from the size request: if a user allocates
 * only one word, it's fully in the Block struct.
 */
inline size_t calc_alloc_size(size_t size);

Block *get_header(word_t *data);

Block *request_mem_from_OS(size_t size);

void free(word_t *data);

Block *first_fit_search(size_t size);

const Block *get_search_start();

size_t get_free_list_size();
/**
 * Mode for searching a free block.
 */
enum struct search_mode_enum {
  first_fit,
  next_fit,
  best_fit,
  free_list,
};

/**
 * Current search mode.
 */
static auto search_mode = search_mode_enum::first_fit;

/**
 * Reset the heap to the original position.
 */
void reset_heap();

/**
 * Initializes the heap, and the search mode.
 */
void init(search_mode_enum mode);

/**
 * Next-fit algorithm.
 *
 * Returns the next free block which fits the size.
 * Updates the `search_start` of success.
 */
Block *next_fit_search(size_t size);

Block *best_fit_search(size_t size);

/**
 * Tries to find a block that fits.
 */
Block *find_block(size_t size);

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
Block *split(Block *block, size_t size);

/**
 * Whether this block can be split.
 */
inline bool can_split(Block *block, size_t size);

/**
 * Allocates a block from the list, splitting if needed.
 */
Block *list_allocate(Block *block, size_t size);

/**
 * Whether we should merge this block.
 */
inline bool can_coalesce(Block *block);

/**
 * Coalesces two adjacent blocks.
 */
Block *coalesce(Block *block);

Block *free_list_search(size_t size);

#endif //UNTITLED_ALLOCATOR_H
