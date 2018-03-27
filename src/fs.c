/*
 * Filement Index
 * Copyright (C) 2018  Martin Kunev <martinkunev@gmail.com>
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

#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base.h"

// TODO replace this with optimized implementation
static void *memrchr(const void *buffer, int c, size_t size)
{
	const char *data = buffer;
	while (size)
		if (data[--size] == c)
			return (void *)(data + size);
	return 0;
}

static int fs_mkdir_parent(char *path, size_t length)
{
	int status;

	char *end = memrchr(path, '/', length);
	if ((!end) || (end == path))
		return ERROR;

	*end = 0;
	status = mkdir(path, 0755);
	if (status < 0)
	{
		if (errno == ENOENT)
			status = fs_mkdir_parent(path, end - path);
		else
			status = ERROR;
	}
	*end = '/';

	return status;
}

int fs_load(char *path, size_t length, int permissions, int truncate)
{
	int result;

	// Try to open the file.
	result = open(path, O_CREAT | O_RDWR, permissions);
	if (result >= 0)
		return result;

	// If open() failed because a parent directory doesn't exist, create it and try to open the file again.
	if (errno != ENOENT)
		return ERROR;
	result = fs_mkdir_parent(path, length);
	if (!result)
		result = open(path, O_CREAT | (truncate ? (O_WRONLY | O_TRUNC) : O_RDWR), permissions);

	return result;
}
