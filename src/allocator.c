//
// Created by panda on 10/6/19.
//

#include "allocator.h"

const struct mem memory = {
    .alloc = alloc,
    .free = free_mem
};

/**
 * Splits the block on two, returns the pointer to the smaller sub-block.
 */
Block *split_default(Block *block, size_t size) {
  size_t space_left = block->size - size;
  Block *block_free = (Block *) (block + size);
  block_free->next = block->next;
  block->next = block_free;
  block_free->size = space_left;
  block_free->used = false;
  return block;
}

Block *split_free_list(Block *block, size_t size) {
  size_t space_left = block->size - size;
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
  ++free_list_size;
  return block;
}

void free_default(word_t *data) {
  Block *block = get_header(data);
//  if (block->size > max_size)
//    free(block);
//  else {
  if (can_coalesce(block)) {
    block = coalesce(block);
  }
  block->used = false;
  // }
}

void free_free_list(word_t *data) {
  Block *block = get_header(data);
//  if (block->size > max_size)
//    free(block);
//  else {
  if (can_coalesce(block)) {
    block = coalesce(block);
  }
  block->used = false;

  //adding to free list
  if (heap_start_free == NULL) {
    heap_start_free = block;
  }
  if (top_free != NULL) {
    top_free->next_free = block;
  }
  block->prev_free = top_free;
  top_free = block;
  //increase number of free blocks;
  ++free_list_size;
  // }
}

void free_segregated(word_t *data) {
  Block *block = get_header(data);
//  if (block->size > max_size)
//    free(block);
//  else {
  block->used = false;
  int bucket = get_bucket(block->size);
  // Init the search.
  if (segregated_starts_free[bucket] == NULL) {
    segregated_starts_free[bucket] = block;
  }
  if (segregated_tops_free[bucket] != NULL) {
    segregated_tops_free[bucket]->next_free = block;
  }
  block->prev_free = segregated_tops_free[bucket];
  segregated_tops_free[bucket] = block;

  ++free_lists_size[bucket];
  //}
}


/**
 * Coalesces two adjacent blocks.
 */

Block *coalesce_default(Block *block) {
  Block *next_block = block->next;
  block->next = next_block->next;
  block->size += next_block->size;
  return block;
}

Block *coalesce_free_list(Block *block) {
  Block *next_block = block->next;
  block->next = next_block->next;
  block->size += next_block->size;
  //union of two blocks to one
  --free_list_size;
  //they both are free and becoming one
  block->next_free = next_block->next_free;
  if (next_block->next_free)
    next_block->next_free->prev_free = block;
  return block;
}

//
//extern inline word_t *alloc_segregated_list(Block *block) {
//  // Init bucket.
//  if (segregated_starts[bucket] == NULL) {
//    segregated_starts[bucket] = block;
//  }
//  // Chain the blocks in the bucket.
//  if (segregated_tops[bucket] != NULL) {
//    segregated_tops[bucket]->next = block;
//  }
//  segregated_tops[bucket] = block;
//}
//
//extern inline word_t *alloc_default(Block *block) {
//  // Init heap.
//  if (heap_start == NULL) {
//    heap_start = block;
//  }
//
//  // Chain the blocks.
//  if (top != NULL) {
//    top->next = block;
//  }
//  top = block;
//}

void *alloc(size_t size) {
  size = align(size);
//  if (size > max_size)
//    return malloc(size);
  Block *block;
  if (block = find_block(size)) {                   // (1)
    return block->data;
  }

  block = request_mem_from_OS(size);

  block->size = size;
  block->used = true;

  if (search_mode == segregated_list) {
    int bucket = get_bucket(size);
    // Init bucket.
    if (segregated_starts[bucket] == NULL) {
      segregated_starts[bucket] = block;
    }
    // Chain the blocks in the bucket.
    if (segregated_tops[bucket] != NULL) {
      segregated_tops[bucket]->next = block;
    }
    segregated_tops[bucket] = block;
  } else {
    // Init heap.
    if (heap_start == NULL) {
      heap_start = block;
    }

    // Chain the blocks.
    if (top != NULL) {
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
  Block *block = heap_start;

  while (block != NULL) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }

    // Found the block:
    return list_allocate(block, size);
  }

  return NULL;
}


Block *segregated_fit_search(size_t size) {
  // Bucket number based on size.
  int bucket = get_bucket(size);
  Block *block = segregated_starts_free[bucket];
  while (block != NULL) {
    // O(n) search.
    if (block->size < size) {
      block = block->next_free;
      continue;
    }

    //removing from free list
    if (block->prev_free)
      block->prev_free->next_free = block->next_free;
    else if (block->next_free)
      block->next_free->prev_free = block->prev_free;
    else {
      segregated_starts_free[bucket] = NULL;
      segregated_tops_free[bucket] = NULL;
    }
    --free_lists_size[bucket];
    // Found the block:
    block->used = true;
    return block;
  }
  return NULL;
}

Block **get_segregated_starts() {
  return segregated_starts;
}

Block **get_segregated_starts_free() {
  return segregated_starts_free;
}

void reset_heap() {
  // Already reset.
  if (heap_start == NULL) {
    return;
  }

  // Roll back to the beginning.
  brk(heap_start);

  heap_start = NULL;
  top = NULL;
  search_start = NULL;
  if (search_mode == free_list) {
    heap_start_free = NULL;
    top_free = NULL;
    free_list_size = 0;
  }
  if (search_mode == segregated_list) {
    heap_start_free = NULL;
    top_free = NULL;
    for (int i = 0; i < 6; ++i) {
      segregated_starts[i] = NULL;
      segregated_tops[i] = NULL;
      segregated_tops_free[i] = NULL;
      segregated_starts_free[i] = NULL;
      free_lists_size[i] = 0;
    }
  }
  find_block = NULL;
  split = NULL;
  coalesce = NULL;
  free_f = NULL;
}

void init(search_mode_enum mode) {
  search_mode = mode;
  reset_heap();
  //TODO make all runtime if (search_mode) checks determined here via different functions
  switch (search_mode) {
    case first_fit:
      find_block = first_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      break;
    case next_fit:
      find_block = next_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      break;
    case best_fit:
      find_block = best_fit_search;
      split = split_default;
      coalesce = coalesce_default;
      free_f = free_default;
      break;
    case free_list:
      find_block = free_list_search;
      split = split_free_list;
      coalesce = coalesce_free_list;
      free_f = free_free_list;
      break;
    case segregated_list:
      find_block = segregated_fit_search;
      free_f = free_segregated;
      break;
  }
}

Block *next_fit_search(size_t size) {
  if (!search_start)
    search_start = heap_start;

  Block *block = search_start;

  while (block != NULL) {
    // O(n) search.
    if (block->used || block->size < size) {
      block = block->next;
      continue;
    }
    search_start = block;
    // Found the block:
    return list_allocate(block, size);
  }

  return NULL;
}

const Block *get_search_start() {
  return search_start;
}

Block *free_list_search(size_t size) {
  Block *block = heap_start_free;
  while (block != NULL) {
    // O(n) search.
    if (block->size < size) {
      block = block->next_free;
      continue;
    }

    //removing from free list
    if (block->prev_free)
      block->prev_free->next_free = block->next_free;
    else if (block->next_free)
      block->next_free->prev_free = block->prev_free;
    else {
      heap_start_free = NULL;
      top_free = NULL;
    }
    --free_list_size;
    // Found the block:
    return list_allocate(block, size);
  }
  return NULL;
}

Block *best_fit_search(size_t size) {
  Block *block = heap_start;
  Block *best_fit = NULL;
  while (block != NULL) {
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
  Block *block = (Block *) sbrk(0);                // (1)

  // OOM.
  if (sbrk(calc_alloc_size(size)) == (void *) -1) {    // (2)
    return NULL;
  }

  return block;
}

Block *get_header(word_t *data) {
  return (Block *) ((char *) data + sizeof(word_t[1]) -
                    sizeof(Block));
}

/**
 * Frees the previously allocated block.
 */
void free_mem(void *data) {
  free_f((word_t *) data);
}

size_t get_free_list_size() {
  return free_list_size;
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

