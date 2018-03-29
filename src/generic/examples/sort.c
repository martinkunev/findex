#include <stdlib.h>

#if defined(UNIT_TESTING)
# include <stdarg.h>
# include <stddef.h>
# include <setjmp.h>
# include <cmocka.h> /* libcmocka */
#endif

#include "sort.h"
#include <heap.g>

// Sorts data in decending order.
void heap_sort(int *restrict data, size_t count)
{
	struct heap heap = {.data = data, .count = count};
    heap_heapify(&heap);
    while (heap.count)
    {
        int value = heap.data[0];
        heap_pop(&heap);
        heap.data[heap.count] = value;
    }
}

#if defined(UNIT_TESTING)

static void test_heap_sort(void **state)
{
	int data[32000];
	size_t i;

	srandom(1);
	for(i = 0; i < sizeof(data) / sizeof(*data); i += 1)
		data[i] = (int)random();

	heap_sort(data, sizeof(data) / sizeof(*data));

	for(i = 1; i < sizeof(data) / sizeof(*data); i += 1)
		assert_true(data[i - 1] >= data[i]);
}

int main()
{
	const struct CMUnitTest tests[] =
	{
		cmocka_unit_test(test_heap_sort),
	};

	return cmocka_run_group_tests(tests, 0, 0);
}

#endif
