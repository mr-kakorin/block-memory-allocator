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
    return block->data;
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
  top = block;

  // User payload:
  return block->data;
}


/**
 * First-fit algorithm.
 *
 * Returns the first free block which fits the size.
 */
Block *first_fit_search(size_t size) {
  auto block = heap_start;

  while (block != nullptr) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }

    // Found the block:
    return list_allocate(block, size);
  }

  return nullptr;
}

inline size_t calc_alloc_size(size_t size) {
  return size + sizeof(Block) - sizeof(std::declval<Block>().data);
}

void reset_heap() {
  // Already reset.
  if (heap_start == nullptr) {
    return;
  }

  // Roll back to the beginning.
  brk(heap_start);

  heap_start = nullptr;
  top = nullptr;
  search_start = nullptr;
  if (search_mode == search_mode_enum::free_list) {
    heap_start_free = nullptr;
    top_free = nullptr;
  }
}

void init(search_mode_enum mode) {
  search_mode = mode;
  reset_heap();
}

Block *next_fit_search(size_t size) {
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
    return list_allocate(block, size);
  }

  return nullptr;
}

const Block *get_search_start() {
  return search_start;
}

Block *find_block(size_t size) {
  switch (search_mode) {
    case search_mode_enum::first_fit:
      return first_fit_search(size);
    case search_mode_enum::next_fit:
      return next_fit_search(size);
    case search_mode_enum::best_fit:
      return best_fit_search(size);
    case search_mode_enum::free_list:
      return free_list_search(size);
  }
}

Block *free_list_search(size_t size) {
  auto block = heap_start_free;
  while (block != nullptr) {
    // O(n) search.
    if (block->size < size) {
      block = block->next_free;
      continue;
    }

    //removing from list
    if (block->prev_free)
      block->prev_free->next_free = block->next_free;

    if (block->next_free)
      block->next_free->prev_free = block->prev_free;
    --free_list_size;
    // Found the block:
    return list_allocate(block, size);
  }
  return nullptr;
}

Block *best_fit_search(size_t size) {
  auto block = heap_start;
  Block *best_fit = nullptr;
  while (block != nullptr) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }
    if (!best_fit || block->size < best_fit->size) {
      best_fit = block;
      block = block->next;
    }
    // Found the block:
    //return block;
  }
  if (best_fit)
    return list_allocate(best_fit, size);
  return best_fit;
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
  if (search_mode == search_mode_enum::free_list) {
    //they both are free
    --free_list_size;
    block->next_free = next_block->next_free;
    if (next_block->next_free)
      next_block->next_free->prev_free = block;

  }
  return block;
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
  if (search_mode == search_mode_enum::free_list) {
    //adding to free list
    if (heap_start_free == nullptr) {
      heap_start_free = block;
    }
    if (top_free != nullptr) {
      top_free->next_free = block;
    }
    block->prev_free = top;
    top_free = block;
    ++free_list_size;
  }
}

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
Block *split(Block *block, size_t size) {
  auto space_left = block->size - size;
  Block *block_free = (Block *) (block + size);
  block_free->next = block->next;
  block->next = block_free;
  block_free->size = space_left;
  block_free->used = false;
  if (search_mode == search_mode_enum::free_list) {
    block_free->prev_free = top_free;
    top_free->next_free = block_free;
    top_free = block_free;
    ++free_list_size;
  }
  return block;
}

size_t get_free_list_size() {
  return free_list_size;
}

/**
 * Whether this block can be split.
 */
inline bool can_split(Block *block, size_t size) {
  return block->size - size >= sizeof(Block);
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