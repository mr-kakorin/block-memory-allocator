//
// Created by panda on 10/6/19.
//

#include <iostream>
#include "allocator.h"
#include <cmath>

inline size_t align(size_t n) {
  return (n + sizeof(word_t) - 1) & ~(sizeof(word_t) - 1);
}

word_t *alloc(size_t size) {
  size = align(size);

  if (auto block = find_block(size)) {                   // (1)
    return list_allocate(block, size)->data;
  }

  auto block = request_mem_from_OS(size);

  block->size = size;
  block->used = true;

  // Init heap.
  if (heap_start == nullptr) {
    heap_start = block;
  }

  // Chain the blocks.
  if (top != nullptr) {
    top->next = block;
  }
  block->prev = top;
  top = block;

  // User payload:
  return block->data;
}


/**
 * First-fit algorithm.
 *
 * Returns the first free block which fits the size.
 */
Block *first_fit(size_t size) {
  auto block = heap_start;

  while (block != nullptr) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }

    // Found the block:
    return block;
  }

  return nullptr;
}

inline size_t calc_alloc_size(size_t size) {
  return size + sizeof(Block) - sizeof(std::declval<Block>().data);
}

void resetHeap() {
  // Already reset.
  if (heap_start == nullptr) {
    return;
  }

  // Roll back to the beginning.
  brk(heap_start);

  heap_start = nullptr;
  top = nullptr;
  search_start = nullptr;
}

void init(search_mode_enum mode) {
  searchMode = mode;
  resetHeap();
}

Block *next_fit(size_t size) {
  if (!search_start)
    search_start = heap_start;

  auto block = search_start;

  while (block != nullptr) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }
    search_start = block;
    // Found the block:
    return block;
  }

  return nullptr;
}

const Block *get_search_start() {
  return search_start;
}

Block *find_block(size_t size) {
  switch (searchMode) {
    case search_mode_enum::first_fit:
      return first_fit(size);
    case search_mode_enum::next_fit:
      return next_fit(size);
    case search_mode_enum::best_fit:
      return best_fit(size);
    case search_mode_enum::free_list:
      return free_list(size);
  }
}

Block *free_list(size_t size) {
  auto block = heap_start;
  while (block != nullptr) {
    // O(n) search.
    if (block->size < size) {
      block = block->next;
      continue;
    }

    //removing from list
    if (block->prev)
      block->prev->next = block->next;

    if (block->next)
      block->next->prev = block->prev;

    // Found the block:
    return list_allocate(block, size);
  }
  return nullptr;
}

Block *best_fit(size_t size) {

  auto block = heap_start;

  size_t min_difference = UINTMAX_MAX;
  Block *bestfit = nullptr;
  while (block != nullptr) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }
    if (block->size - size < min_difference) {
      bestfit = block;
      min_difference = block->size - size;
      block = block->next;
    }
    // Found the block:
    //return block;
  }

  return bestfit;
}

Block *request_mem_from_OS(size_t size) {
  // Current heap break.
  auto block = (Block *) sbrk(0);                // (1)

  // OOM.
  if (sbrk(calc_alloc_size(size)) == (void *) -1) {    // (2)
    return nullptr;
  }

  return block;
}

Block *get_header(word_t *data) {
  return (Block *) ((char *) data + sizeof(std::declval<Block>().data) -
                    sizeof(Block));
}


inline bool can_coalesce(Block *block) {
  return block->next && !block->next->used;
}

/**
 * Coalesces two adjacent blocks.
 */
Block *coalesce(Block *block) {
  auto next_block = block->next;
  block->next = next_block->next;
  block->size += next_block->size;
}

/**
 * Frees the previously allocated block.
 */
void free(word_t *data) {
  auto block = get_header(data);
  if (can_coalesce(block)) {
    block = coalesce(block);
  }
  block->used = false;
  if (searchMode == search_mode_enum::free_list) {
    //adding to free list
    if (block->prev)
      block->prev->next = block;

    if (block->next)
      block->next->prev = block;
  }
}

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
Block *split(Block *block, size_t size) {
  auto space_left = block->size - size;
  Block *block_left = (Block *) (block + size);
  block_left->next = block->next;
  block->next = block_left;
  block_left->size = space_left;
  block_left->used = false;
  return block;
}

/**
 * Whether this block can be split.
 */
inline bool can_split(Block *block, size_t size) {
  auto space_left = block->size - size;
  return space_left > size && align(space_left) - space_left == 0;
}

/**
 * Allocates a block from the list, splitting if needed.
 */
Block *list_allocate(Block *block, size_t size) {
// Split the larger block, reusing the free part.
  if (can_split(block, size)) {
    block = split(block, size);
  }

  block->used = true;
  block->size = size;

  return block;
}