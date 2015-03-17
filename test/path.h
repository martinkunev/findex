/*
 * Filement Index
 * Copyright (C) 2015  Martin Kunev <martinkunev@gmail.com>
 *
 * This file is part of Filement Index.
 *
 * Filement Index is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 3 of the License.
 *
 * Filement Index is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Filement Index.  If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(TEST)

#include <check.h>

static char test_cwd[PATH_SIZE_LIMIT];
static size_t test_cwd_size;

#define getcwd(buffer, size) (char *)dupalloc(test_cwd, test_cwd_size)

static void test_set_cwd(void)
{
	bytes_define(current, "/home/martin\0");
	memcpy(test_cwd, current.data, current.size);
	test_cwd_size = current.size;
}

static void check(const bytes_t *restrict relative, const bytes_t *restrict answer)
{
	char path[3][PATH_SIZE_LIMIT];
	size_t path_length;

	unsigned char canary[PATH_SIZE_LIMIT];
	int status;

	// Use 10100101 as canary.
	memset(canary, '\xa5', sizeof(canary));
	memcpy(path[0], canary, sizeof(canary));
	memcpy(path[1], canary, sizeof(canary));
	memcpy(path[2], canary, sizeof(canary));

	status = normalize(path[1], &path_length, (const char *)relative->data, relative->size); // TODO fix the cast
	ck_assert_uint_eq(status, 0);
	ck_assert_uint_eq(path_length, answer->size);
	ck_assert(!memcmp(path[0], canary, sizeof(canary)));
	ck_assert(!memcmp(path[1], answer->data, answer->size));
	ck_assert(!memcmp(path[2], canary, sizeof(canary)));
}

START_TEST(test_normalize_root)
{
	test_set_cwd();
	bytes_define(relative, "/");
	bytes_define(answer, "/");
	check((const bytes_t *)&relative, (const bytes_t *)&answer);
}
END_TEST

START_TEST(test_normalize_simple)
{
	test_set_cwd();
	bytes_define(relative, "x");
	bytes_define(answer, "/home/martin/x/");
	check((const bytes_t *)&relative, (const bytes_t *)&answer);
}
END_TEST

START_TEST(test_normalize_current)
{
	test_set_cwd();
	bytes_define(relative, ".");
	bytes_define(answer, "/home/martin/");
	check((const bytes_t *)&relative, (const bytes_t *)&answer);
}
END_TEST

START_TEST(test_normalize_parent)
{
	test_set_cwd();
	bytes_define(relative, "../dir/");
	bytes_define(answer, "/home/dir/");
	check((const bytes_t *)&relative, (const bytes_t *)&answer);
}
END_TEST

int main(void)
{
	unsigned failed;

	Suite *suite = suite_create("path");
	TCase *tc;

	tc = tcase_create("normalize");
	tcase_add_test(tc, test_normalize_root);
	tcase_add_test(tc, test_normalize_simple);
	tcase_add_test(tc, test_normalize_current);
	tcase_add_test(tc, test_normalize_parent);
	suite_add_tcase(suite, tc);

	SRunner *runner = srunner_create(suite);
	srunner_set_fork_status(runner, CK_NOFORK);
	srunner_run_all(runner, CK_VERBOSE);
	failed = srunner_ntests_failed(runner);
	srunner_free(runner);
	return failed;
}

#endif
