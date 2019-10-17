//
// Created by panda on 10/6/19.
//

#include <iostream>
#include "allocator.h"

/**
 * Allocates a block from the list, splitting if needed.
 */
inline Block *list_allocate_segregated(Block *block, size_t size) {
  block->used = true;
  return block;
}


Block *list_allocate_default(Block *block, size_t size) {
// Split the larger block, reusing the free part.
  if (can_split(block, size)) {
    block = split(block, size);
  }

  block->used = true;
  block->size = size;

  return block;
}

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
Block *split_default(Block *block, size_t size) {
  auto space_left = block->size - size;
  Block *block_free = (Block *) (block + size);
  block_free->next = block->next;
  block->next = block_free;
  block_free->size = space_left;
  block_free->used = false;
  return block;
}

Block *split_free_list(Block *block, size_t size) {
  auto space_left = block->size - size;
  Block *block_free = (Block *) (block + size);
  block_free->next = block->next;
  block->next = block_free;
  block_free->size = space_left;
  block_free->used = false;
  block_free->prev_free = top_free;
  top_free->next_free = block_free;
  top_free = block_free;
  //we increase here because block will be used later in free_list_search
  //where this value will be decremented
  ++(*free_list_size);
  return block;
}


void free_default(word_t *data) {
  auto block = get_header(data);
  if (can_coalesce(block)) {
    block = coalesce(block);
  }
  block->used = false;
}

void free_free_list(word_t *data) {
  auto block = get_header(data);
  if (can_coalesce(block)) {
    block = coalesce(block);
  }
  block->used = false;

  //adding to free list
  if (heap_start_free == nullptr) {
    heap_start_free = block;
  }
  if (top_free != nullptr) {
    top_free->next_free = block;
  }
  block->prev_free = top_free;
  top_free = block;
  //increase number of free blocks;
  ++(*free_list_size);
}

void free_segregated(word_t *data) {
  auto block = get_header(data);
  block->used = false;
  auto bucket = get_bucket(block->size);
  // Init the search.
  if (segregated_starts_free[bucket] == nullptr) {
    segregated_starts_free[bucket] = block;
  }
  if (segregated_tops_free[bucket] != nullptr) {
    segregated_tops_free[bucket]->next_free = block;
  }
  block->prev_free = segregated_tops_free[bucket];
  segregated_tops_free[bucket] = block;

  ++free_lists_size[bucket];
}


/**
 * Coalesces two adjacent blocks.
 */

Block *coalesce_default(Block *block) {
  auto next_block = block->next;
  block->next = next_block->next;
  block->size += next_block->size;
  return block;
}

Block *coalesce_free_list(Block *block) {
  auto next_block = block->next;
  block->next = next_block->next;
  block->size += next_block->size;
  //union of two blocks to one
  --(*free_list_size);
  //they both are free and becoming one
  block->next_free = next_block->next_free;
  if (next_block->next_free)
    next_block->next_free->prev_free = block;
  return block;
}


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

  if (search_mode == search_mode_enum::segregated_list) {
    auto bucket = get_bucket(size);
    // Init bucket.
    if (segregated_starts[bucket] == nullptr) {
      segregated_starts[bucket] = block;
    }
    // Chain the blocks in the bucket.
    if (segregated_tops[bucket] != nullptr) {
      segregated_tops[bucket]->next = block;
    }
    segregated_tops[bucket] = block;
  } else {
    // Init heap.
    if (heap_start == nullptr) {
      heap_start = block;
    }

    // Chain the blocks.
    if (top != nullptr) {
      top->next = block;
    }
    top = block;
  }
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


inline int get_bucket(size_t size) {
  return size / sizeof(word_t) - 1;
}

Block *segregated_fit_search(size_t size) {
  // Bucket number based on size.
  auto bucket = get_bucket(size);
  auto original_heap_start_free = heap_start_free;
  auto original_heap_top_free = top_free;
  auto original_free_list_size = free_list_size;
  // Init the search.
  heap_start_free = segregated_starts_free[bucket];
  top_free = segregated_tops_free[bucket];
  free_list_size = free_lists_size[bucket];
  // Use first-fit here, but can be any:
  auto block = free_list_search(size);

  heap_start_free = original_heap_start_free;
  top_free = original_heap_top_free;
  free_list_size = original_free_list_size;
  return block;
}

Block **get_segregated_starts() {
  return segregated_starts;
}

Block **get_segregated_starts_free() {
  return segregated_starts_free;
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
    *free_list_size = 0;
  }
  if (search_mode == search_mode_enum::segregated_list) {
    heap_start_free = nullptr;
    top_free = nullptr;
    for (int i = 0; i < 6; ++i) {
      segregated_starts[i] = nullptr;
      segregated_tops[i] = nullptr;
      segregated_tops_free[i] = nullptr;
      segregated_starts_free[i] = nullptr;
      *free_lists_size[i] = 0;
    }
  }
  find_block = nullptr;
  split = nullptr;
  coalesce = nullptr;
  free_f = nullptr;
  list_allocate = nullptr;
}

void init(search_mode_enum mode) {
  search_mode = mode;
  reset_heap();
  //TODO make all runtime if (search_mode) checks determined here via different functions
  switch (search_mode) {
    case search_mode_enum::first_fit:
      find_block = first_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      list_allocate = list_allocate_default;
      break;
    case search_mode_enum::next_fit:
      find_block = next_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      list_allocate = list_allocate_default;
      break;
    case search_mode_enum::best_fit:
      find_block = best_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      list_allocate = list_allocate_default;
      break;
    case search_mode_enum::free_list:
      find_block = free_list_search;
      split = split_free_list;
      coalesce = coalesce_free_list;
      free_f = free_free_list;
      list_allocate = list_allocate_default;
      break;
    case search_mode_enum::segregated_list:
      find_block = segregated_fit_search;
      free_f = free_segregated;
      list_allocate = list_allocate_segregated;
      break;
  }
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
    --(*free_list_size);
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
 * Frees the previously allocated block.
 */
void free(word_t *data) {
  free_f(data);
}

size_t get_free_list_size() {
  return *free_list_size;
}

/**
 * Whether this block can be split.
 */
inline bool can_split(Block *block, size_t size) {
  return block->size - size >= sizeof(Block);
}

