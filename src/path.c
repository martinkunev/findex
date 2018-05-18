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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base.h"
#include "path.h"

#define PATH_PREFIX "/.cache/filement/" /* relative path to directory storing database */

enum {FILENAME_SIZE_LIMIT = 255};

// Total path length with NUL is limited to PATH_SIZE_LIMIT.
// Path component length is limited to FILENAME_SIZE_LIMIT.
int path_init(struct path_buffer *restrict path)
{
	const char *home;
	size_t home_length;

	home = getenv("HOME");
	if (!home)
		return ERROR; // TODO error code

	home_length = strlen(home);
	if ((home_length + sizeof(PATH_PREFIX) - 1 + FILENAME_SIZE_LIMIT + 1) > PATH_SIZE_LIMIT)
		return ERROR_UNSUPPORTED;

	memcpy(path->data, home, home_length);
	memcpy(path->data + home_length, PATH_PREFIX, sizeof(PATH_PREFIX) - 1);

	path->prefix_length = home_length + sizeof(PATH_PREFIX) - 1;

	return 0;
}

size_t path_set(struct path_buffer *restrict path, const char *restrict name, size_t name_length)
{
	size_t length;

	assert(name_length <= FILENAME_SIZE_LIMIT);

	memcpy(path->data + path->prefix_length, name, name_length);
	length = path->prefix_length + name_length;
	path->data[length] = 0;

	return length;
}

// Generates a normalized absolute path corresponding to a given path (getting current directory if necessary). Terminates the string with NUL.
int normalize(char path[static restrict PATH_SIZE_LIMIT], size_t *restrict path_length, const char *restrict raw, size_t raw_length)
{
	size_t length;
	size_t start, index;
	size_t offset;

	assert(raw_length);

	// Prepare the path for normalization.
	if (raw[0] != '/') // the input is relative path
	{
		char *cwd;
		size_t cwd_length;

		cwd = getcwd(0, 0);
		if (!cwd)
			return ERROR; // TODO better error
		cwd_length = strlen(cwd);

		// Copy current directory path in path.
		// Add terminating slash if cwd is not the root directory.
		length = cwd_length + (cwd_length > 1);
		if (length > PATH_SIZE_LIMIT)
		{
			free(cwd);
			return ERROR_UNSUPPORTED;
		}
		memcpy(path, cwd, cwd_length);
		free(cwd);
		if (cwd_length > 1)
			path[cwd_length] = '/';
	}
	else // absolute path
	{
		path[0] = '/';
		length = 1;
	}

	assert(length >= 1);
	assert(path[0] == '/');
	assert(path[length - 1] == '/');

	// At the position of each slash, store path component length.
	// Keep the initial slash untouched.
	// Make start point to the slash before the last path component.
	index = 1;
	while (index < length)
	{
		char *component_start, *component_end;

		component_start = path + index;
		component_end = memchr(component_start, '/', length - index);

		assert(component_end);

		if ((component_end - component_start) > 255)
			return ERROR_UNSUPPORTED; // path component too long

		*component_end = component_end - component_start;
		index = component_end + 1 - path;
	}

	const char *raw_end = raw + raw_length;
	size_t component_length;

	// Normalize path by removing . and .. components and slash repetitions.
	for(; raw < raw_end; raw += component_length + 1)
	{

		// Find the end of the current path component.
		const char *end = memchr(raw, '/', raw_end - raw);
		if (!end)
			end = raw_end;
		component_length = end - raw;

		switch (component_length)
		{
		case 0: // skip repeated /
			continue;

		case 1: // check for .
			if (raw[0] != '.')
				break;
			continue;

		case 2: // check for ..
			if ((raw[0] != '.') || (raw[1] != '.'))
				break;
			if (index > 1) // in the root directory .. points to the same directory
				index -= path[index - 1] + 1;
			continue;
		}

		if (index + component_length + 1 > PATH_SIZE_LIMIT)
			return ERROR_UNSUPPORTED;

		memcpy(path + index, raw, component_length);
		path[index + component_length] = component_length;
		index += component_length + 1;
	}

	index -= 1;
	*path_length = index;

	// Restore path component separators to slashes.
	while (index)
	{
		component_length = path[index];
		path[index] = '/';
		index -= 1 + component_length;
	}

	path[*path_length] = 0;

	return 0;
}
