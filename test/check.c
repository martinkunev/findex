#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "check.h"
#include "path.h"

int main(void)
{
	const struct CMUnitTest tests[] =
	{
		cmocka_unit_test(test_normalize_root),
		cmocka_unit_test(test_normalize_simple),
		cmocka_unit_test(test_normalize_current),
		cmocka_unit_test(test_normalize_parent),
	};
	return cmocka_run_group_tests(tests, 0, 0);
}
