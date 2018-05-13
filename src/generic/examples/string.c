#include <stdlib.h>
#include <string.h>

#if defined(UNIT_TESTING)
# include <stdarg.h>
# include <stddef.h>
# include <setjmp.h>
# include <cmocka.h> /* libcmocka */
#endif

#include "string.h"

#define NAME_CAT_EXPAND(a, b) a ## b
#define NAME_CAT(a, b) NAME_CAT_EXPAND(a, b)
#define NAME(suffix) NAME_CAT(ARRAY_NAME, suffix)
int NAME(_append)(struct ARRAY_NAME *restrict string, const ARRAY_TYPE *data, size_t count)
{
	int status = NAME(_expand)(string, string->count + count);
	if (status < 0)
		return status;
	memcpy(string->data + string->count, data, count);
	string->count += count;
	return status;
}
#undef NAME
#undef NAME_CAT
#undef NAME_CAT_EXPAND

#include "array.g"

#if defined(UNIT_TESTING)

static void test_array_expand(void **state)
{
	struct string s = {0};

	string_expand(&s, 12);

	assert_int_equal(s.count, 0);
	assert_int_equal(s.capacity, 16);
	assert_ptr_not_equal(s.data, 0);

	free(s.data);
}

static void test_array_stream(void **state)
{
# define APPEND(string, data) string_append((string), (data), sizeof(data) - 1)
	struct string s = {0};

	if (APPEND(&s, "hello") < 0)
		abort();
	if (APPEND(&s, " ") < 0)
		abort();
	if (APPEND(&s, "world") < 0)
		abort();
	if (APPEND(&s, "!") < 0)
		abort();

	assert_memory_equal(s.data, "hello world!", 12);

	free(s.data);
# undef APPEND
}

int main()
{
	const struct CMUnitTest tests[] =
	{
		cmocka_unit_test(test_array_expand),
		cmocka_unit_test(test_array_stream),
	};

	return cmocka_run_group_tests(tests, 0, 0);
}

#endif
