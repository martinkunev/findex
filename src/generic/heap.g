/*
 * Conquest of Levidon
 * Copyright (C) 2018  Martin Kunev <martinkunev@gmail.com>
 *
 * This file is part of Conquest of Levidon.
 *
 * Conquest of Levidon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 3 of the License.
 *
 * Conquest of Levidon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Conquest of Levidon.  If not, see <http://www.gnu.org/licenses/>.
 */

// The arguments with which the macro functions are called are guaranteed to produce no side effects.

#if !defined(HEAP_NAME)
# define HEAP_NAME heap
#endif

#if !defined(HEAP_TYPE)
# define HEAP_TYPE void *
#endif

#if !defined(HEAP_GLOBAL) || defined(HEAP_SOURCE)
// Returns whether a has higher priority than b.
# if !defined(HEAP_ABOVE)
#  define HEAP_ABOVE(a, b) ((a) >= (b))
# endif

// Called after an element is placed or moved to a given index in the heap.
# if !defined(HEAP_UPDATE)
#  define HEAP_UPDATE(heap, index)
# endif
#endif

#define NAME_CAT_EXPAND(a, b) a ## b
#define NAME_CAT(a, b) NAME_CAT_EXPAND(a, b)
#define NAME(suffix) NAME_CAT(HEAP_NAME, suffix)

#define STRING_EXPAND(string) #string
#define STRING(string) STRING_EXPAND(string)

#if defined(HEAP_GLOBAL)
# define STATIC
#else
# define STATIC static
#endif

/*#if defined(HEADER)
# include <stddef.h>
#else
# include <stdlib.h>
#endif*/

#if !defined(HEAP_GLOBAL) || !defined(HEAP_SOURCE)

struct HEAP_NAME
{
	HEAP_TYPE *data; // Array with the elements.
	size_t count; // Number of elements actually in the heap.
};

STATIC void NAME(_push)(struct HEAP_NAME *heap, HEAP_TYPE value);
STATIC void NAME(_pop)(struct HEAP_NAME *heap);
STATIC void NAME(_emerge)(struct HEAP_NAME *heap, size_t index);
STATIC void NAME(_heapify)(struct HEAP_NAME *heap);

#endif

#if !defined(HEAP_GLOBAL) || defined(HEAP_SOURCE)

// Push element to the heap.
STATIC void NAME(_push)(struct HEAP_NAME *heap, HEAP_TYPE value)
{
	size_t index, parent;

	// Find out where to put the element and put it.
	for(index = heap->count++; index; index = parent)
	{
		parent = (index - 1) / 2;
		if (HEAP_ABOVE(heap->data[parent], value)) break;
		heap->data[index] = heap->data[parent];
		HEAP_UPDATE(heap, index);
	}
	heap->data[index] = value;
	HEAP_UPDATE(heap, index);
}

// Removes the biggest element from the heap.
STATIC void NAME(_pop)(struct HEAP_NAME *heap)
{
	size_t index, swap, other;

	// Remove the biggest element.
	HEAP_TYPE temp = heap->data[--heap->count];

	// Reorder the elements.
	index = 0;
	while (1)
	{
		// Find which child to swap with.
		swap = index * 2 + 1;
		if (swap >= heap->count) break; // If there are no children, the heap is reordered.
		other = swap + 1;
		if ((other < heap->count) && HEAP_ABOVE(heap->data[other], heap->data[swap])) swap = other;
		if (HEAP_ABOVE(temp, heap->data[swap])) break; // If the bigger child is less than or equal to its parent, the heap is reordered.

		heap->data[index] = heap->data[swap];
		HEAP_UPDATE(heap, index);
		index = swap;
	}
	heap->data[index] = temp;
	HEAP_UPDATE(heap, index);
}

// Move an element closer to the front of the heap.
STATIC void NAME(_emerge)(struct HEAP_NAME *heap, size_t index) // TODO ? rename to heap_sift_up
{
	size_t parent;

	HEAP_TYPE temp = heap->data[index];

	for(; index; index = parent)
	{
		parent = (index - 1) / 2;
		if (HEAP_ABOVE(heap->data[parent], temp)) break;
		heap->data[index] = heap->data[parent];
		HEAP_UPDATE(heap, index);
	}
	heap->data[index] = temp;
	HEAP_UPDATE(heap, index);
}

// Heapifies a non-empty array.
STATIC void NAME(_heapify)(struct HEAP_NAME *heap)
{
	unsigned item, index, swap, other;
	HEAP_TYPE temp;

	if (heap->count < 2) return;

	// Move each non-leaf element down in its subtree until it satisfies the heap property.
	item = (heap->count / 2) - 1;
	while (1)
	{
		// Find the position of the current element in its subtree.
		temp = heap->data[item];
		index = item;
		while (1)
		{
			// Find the child to swap with.
			swap = index * 2 + 1;
			if (swap >= heap->count) break; // If there are no children, the element is placed properly.
			other = swap + 1;
			if ((other < heap->count) && HEAP_ABOVE(heap->data[other], heap->data[swap])) swap = other;
			if (HEAP_ABOVE(temp, heap->data[swap])) break; // If the bigger child is less than or equal to the parent, the element is placed properly.

			heap->data[index] = heap->data[swap];
			HEAP_UPDATE(heap, index);
			index = swap;
		}
		if (index != item)
		{
			heap->data[index] = temp;
			HEAP_UPDATE(heap, index);
		}

		if (!item) return;
		--item;
	}
}

#endif

#undef STATIC

#undef STRING
#undef STRING_EXPAND

#undef NAME
#undef NAME_CAT
#undef NAME_CAT_EXPAND

#if defined(HEAP_GLOBAL) && !defined(HEAP_SOURCE)
# define HEAP_SOURCE
#else
# undef HEAP_UPDATE
# undef HEAP_ABOVE
# undef HEAP_TYPE
# undef HEAP_NAME

# undef HEAP_SOURCE
# undef HEAP_GLOBAL
#endif
