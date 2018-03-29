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

#if !defined(ARRAY_NAME)
# define ARRAY_NAME array
#endif

#if !defined(ARRAY_TYPE)
# define ARRAY_TYPE void *
#endif

#define NAME_CAT_EXPAND(a, b) a ## b
#define NAME_CAT(a, b) NAME_CAT_EXPAND(a, b)
#define NAME(suffix) NAME_CAT(ARRAY_NAME, suffix)

#define STRING_EXPAND(string) #string
#define STRING(string) STRING_EXPAND(string)

#if defined(ARRAY_GLOBAL)
# define STATIC
#else
# define STATIC static
#endif

#if !defined(ARRAY_GLOBAL) || !defined(ARRAY_SOURCE)
enum {ARRAY_SIZE_BASE = 8}; /* WARNING: This must be a power of 2 so that capacity is calculated correctly. */

struct ARRAY_NAME
{
	size_t count;
	size_t capacity;
	ARRAY_TYPE *data;
};

STATIC int NAME(_expand)(struct ARRAY_NAME *restrict array, size_t count);
#endif

#if !defined(ARRAY_GLOBAL) || defined(ARRAY_SOURCE)

STATIC int NAME(_expand)(struct ARRAY_NAME *restrict array, size_t count)
{
	size_t capacity;
	ARRAY_TYPE *buffer;

	if (array->capacity >= count)
		return 0;

	// Round count up to the next power of 2 that is >= ARRAY_SIZE_BASE.
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
	capacity = 1 << (sizeof(array->capacity) * 8 - __builtin_clz((count - 1) | (ARRAY_SIZE_BASE - 1)));
#else
	for(capacity = ARRAY_SIZE_BASE; capacity < count; capacity *= 2)
		;
#endif

	buffer = realloc(array->data, capacity * sizeof(*array->data));
	if (!buffer)
		return -1;
	array->data = buffer;
	array->capacity = capacity;

	return 0;
}

#endif

#undef STATIC

#undef STRING
#undef STRING_EXPAND

#undef NAME
#undef NAME_CAT
#undef NAME_CAT_EXPAND

#if defined(ARRAY_GLOBAL) && !defined(ARRAY_SOURCE)
# define ARRAY_SOURCE
#else
# undef ARRAY_TYPE
# undef ARRAY_NAME

# undef ARRAY_SOURCE
# undef ARRAY_GLOBAL
#endif
