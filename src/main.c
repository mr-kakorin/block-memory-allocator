#include "allocator.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>


int main() {
  init(first_fit);
  word_t *p1 = memory.alloc(3);                        // (1)
  Block *p1b = get_header(p1);
  assert(p1b->size == sizeof(word_t));
  // --------------------------------------
  // Test case 2: Exact amount of aligned bytes
  //

  word_t *p2 = memory.alloc(8);                        // (2)
  Block *p2b = get_header(p2);
  assert(p2b->size == 8);

  memory.free(p2);
  assert(!p2b->used);

  word_t *p3 = memory.alloc(8);
  Block *p3b = get_header(p3);
  assert(p3b->size == 8);
  assert(p3b == p2b);  // Reused!

  // Init the heap, and the searching algorithm.
  init(next_fit);

// --------------------------------------
// Test case 5: Next search start position
//

// [[8, 1], [8, 1], [8, 1]]
  memory.alloc(8);
  memory.alloc(8);
  memory.alloc(8);

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
  word_t *o1 = memory.alloc(16);
  word_t *o2 = memory.alloc(16);

// [[8, 1], [8, 1], [8, 1], [16, 0], [16, 0]]
  memory.free(o1);
  memory.free(o2);

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 0]]
  word_t *o3 = memory.alloc(16);
// Start position from o3:
  assert(get_search_start() == get_header(o3));

// [[8, 1], [8, 1], [8, 1], [16, 1], [16, 1]]
//                           ^ start here
  memory.alloc(16);

  init(best_fit);

// --------------------------------------
// Test case 6: Best-fit search
//

// [[8, 1], [64, 1], [8, 1], [16, 1]]
  memory.alloc(8);
  word_t *z1 = memory.alloc(64);
  memory.alloc(8);
  word_t *z2 = memory.alloc(16);

// memory.free the last 16
  memory.free(z2);

// memory.free 64:
  memory.free(z1);

// [[8, 1], [64, 0], [8, 1], [16, 0]]

// Reuse the last 16 block:
  word_t *z3 = memory.alloc(16);
  assert(get_header(z3) == get_header(z2));

// [[8, 1], [64, 0], [8, 1], [16, 1]]


  // Reuse 64, splitting it to 16, and 48
  z3 = memory.alloc(16);
  assert(get_header(z3) == get_header(z1));

  init(free_list);

  memory.alloc(8);
  memory.alloc(8);
  word_t *v1 = memory.alloc(16);
  memory.alloc(8);

  memory.free(v1);
  assert(get_free_list_size() == 1);

  word_t *v2 = memory.alloc(16);
  assert(get_free_list_size() == 0);
  assert(get_header(v1) == get_header(v2));


  init(segregated_list);

  word_t *s1 = memory.alloc(3);
  word_t *s2 = memory.alloc(8);

  assert(get_header(s1) == get_segregated_starts()[0]);
  assert(get_header(s2) == get_segregated_starts()[0]->next);

  word_t *s3 = memory.alloc(16);
  assert(get_header(s3) == get_segregated_starts()[1]);

  word_t *s4 = memory.alloc(8);
  assert(get_header(s4) == get_segregated_starts()[0]->next->next);

  word_t *s5 = memory.alloc(32);
  assert(get_header(s5) == get_segregated_starts()[3]);

  memory.free(s1);

  word_t *s6 = memory.alloc(5);
  assert(get_header(s6) == get_header(s1));
  memory.free(s2);
  word_t *s7 = memory.alloc(8);
  assert(get_header(s7) == get_header(s2));
  memory.free(s3);
  init(segregated_list);
  //puts("\nAll assertions passed!\n");
  const int ntests = 256;
  const int ntests_internal = 10000;
  clock_t begin = clock();
  for (int i = 1; i < ntests; ++i) {
    for (int j = 0; j < ntests_internal; ++j) {
      int *a = (int *) memory.alloc(i);
      memory.free(a);
    }
  }

  clock_t end = clock();
  double t1, t2;
  printf("Elapsed time: %lf\n", t1 = (double) (end - begin) / CLOCKS_PER_SEC);

  begin = clock();
  for (int i = 1; i < ntests; ++i) {
    for (int j = 0; j < ntests_internal; ++j) {
      int *a = malloc(sizeof(int) * i);
      free(a);
    }
  }
  end = clock();
  printf("Elapsed time: %lf\n", t2 = (double) (end - begin) / CLOCKS_PER_SEC);
  if (t1 > t2) {
    printf("Slower: %lf\n", t1 / t2);
  } else {
    printf("Faster: %lf\n", t2 / t1);
  }
  return 0;
}