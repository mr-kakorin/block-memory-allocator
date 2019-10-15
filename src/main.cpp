#include <iostream>
#include "allocator.h"
#include <assert.h>

int main() {
  init(search_mode_enum::first_fit);
  auto p1 = alloc(3);                        // (1)
  auto p1b = get_header(p1);
  assert(p1b->size == sizeof(word_t));
  // --------------------------------------
  // Test case 2: Exact amount of aligned bytes
  //

  auto p2 = alloc(8);                        // (2)
  auto p2b = get_header(p2);
  assert(p2b->size == 8);

  free(p2);
  assert(!p2b->used);

  auto p3 = alloc(8);
  auto p3b = get_header(p3);
  assert(p3b->size == 8);
  assert(p3b == p2b);  // Reused!

  // Init the heap, and the searching algorithm.
  init(search_mode_enum::next_fit);

// --------------------------------------
// Test case 5: Next search start position
//

// [[8, 1], [8, 1], [8, 1]]
  alloc(8);
  alloc(8);
  alloc(8);

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  auto o1 = alloc(16);
  auto o2 = alloc(16);

// [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
  free(o1);
  free(o2);

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
  auto o3 = alloc(16);
// Start position from o3:
  assert(get_search_start() == get_header(o3));

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
//                           ^ start here
  alloc(16);

  init(search_mode_enum::best_fit);

// --------------------------------------
// Test case 6: Best-fit search
//

// [[8, 1], [64, 1], [8, 1], [16, 1]]
  alloc(8);
  auto z1 = alloc(64);
  alloc(8);
  auto z2 = alloc(16);

// Free the last 16
  free(z2);

// Free 64:
  free(z1);

// [[8, 1], [64, 0], [8, 1], [16, 0]]

// Reuse the last 16 block:
  auto z3 = alloc(16);
  assert(get_header(z3) == get_header(z2));

// [[8, 1], [64, 0], [8, 1], [16, 1]]


  // Reuse 64, splitting it to 16, and 48
  z3 = alloc(16);
  assert(get_header(z3) == get_header(z1));

  init(search_mode_enum::free_list);

  alloc(8);
  alloc(8);
  auto v1 = alloc(16);
  alloc(8);

  free(v1);
  assert(get_free_list_size() == 1);

  auto v2 = alloc(16);
  assert(get_free_list_size() == 0);
  assert(get_header(v1) == get_header(v2));

  return 0;
}