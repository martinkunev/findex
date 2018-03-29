#define HEAP_GLOBAL
#define HEAP_TYPE int
#define HEAP_ABOVE(a, b) ((a) <= (b))
#include <heap.g>

void heap_sort(int *restrict data, size_t count);
