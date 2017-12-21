/*
 * Filement Index
 * Copyright (C) 2017  Martin Kunev <martinkunev@gmail.com>
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

#include <stdlib.h>
#include <string.h>

#include <base.h>
#include <path.h>

char *__wrap_getcwd(char *buf, size_t size)
{
	check_expected(buf);
	check_expected(size);
	return (char *)mock();
}

void __wrap_free(void *ptr)
{
	check_expected(ptr);
}

static void check(const struct bytes *restrict relative, const struct bytes *restrict answer)
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
	assert_int_equal(status, 0);
	assert_int_equal(path_length, answer->size);
	assert_memory_equal(path[0], canary, sizeof(canary));
	assert_memory_equal(path[1], answer->data, answer->size);
	assert_memory_equal(path[2], canary, sizeof(canary));
}

static void test_normalize_root(void **state)
{
	struct bytes *relative = bytes("/");
	struct bytes *answer = bytes("/");
	check(relative, answer);
}

static void test_normalize_simple(void **state)
{
	const char *directory = "/home/martin";

	expect_value(__wrap_getcwd, buf, 0);
	expect_value(__wrap_getcwd, size, 0);
	will_return(__wrap_getcwd, directory);

	expect_value(__wrap_free, ptr, directory);

	struct bytes *relative = bytes("x");
	struct bytes *answer = bytes("/home/martin/x/");
	check(relative, answer);
}

static void test_normalize_current(void **state)
{
	const char *directory = "/home/martin";

	expect_value(__wrap_getcwd, buf, 0);
	expect_value(__wrap_getcwd, size, 0);
	will_return(__wrap_getcwd, directory);

	expect_value(__wrap_free, ptr, directory);

	struct bytes *relative = bytes(".");
	struct bytes *answer = bytes("/home/martin/");
	check(relative, answer);
}

static void test_normalize_parent(void **state)
{
	const char *directory = "/home/martin";

	expect_value(__wrap_getcwd, buf, 0);
	expect_value(__wrap_getcwd, size, 0);
	will_return(__wrap_getcwd, directory);

	expect_value(__wrap_free, ptr, directory);

	struct bytes *relative = bytes("../dir/");
	struct bytes *answer = bytes("/home/dir/");
	check(relative, answer);
}
