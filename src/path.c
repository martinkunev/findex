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
#include <unistd.h>

#include "base.h"
#include "path.h"

#define DB_PATH "/.cache/filement" /* database path relative to the user's home directory */

int db_path_init(char path[static PATH_SIZE_LIMIT])
{
	const char *home;
	size_t home_length;

	home = getenv("HOME");
	if (!home) return ERROR_MISSING;
	home_length = strlen(home);
	if ((home_length + sizeof(DB_PATH)) > PATH_SIZE_LIMIT) return ERROR_MISSING;

	memcpy(path, home, home_length);
	memcpy(path + home_length, DB_PATH, sizeof(DB_PATH) - 1);
	path[home_length + sizeof(DB_PATH) - 1] = 0;

	return 0;
}

// Generates a normalized absolute path corresponding to the relative path.
int normalize(char path[static restrict PATH_SIZE_LIMIT], size_t *restrict path_length, const char *restrict raw, size_t raw_length)
{
	size_t length;
	size_t start, index;
	size_t offset;

	// Prepare the path for normalization.
	if (raw[0] != '/') // the input is relative path
	{
		char *cwd;
		size_t cwd_length;

		cwd = getcwd(0, 0);
		if (!cwd) abort();
		cwd_length = strlen(cwd);

		// Copy current directory path in path.
		// Put terminating slash if cwd is not the root directory.
		length = ((cwd_length > 1) ? (cwd_length + 1) : cwd_length);
		if (length > PATH_SIZE_LIMIT)
		{
			free(cwd);
			return ERROR_UNSUPPORTED;
		}
		memcpy(path, cwd, cwd_length);
		free(cwd);
		if (cwd_length > 1) path[cwd_length] = '/';
	}
	else // absolute path
	{
		path[0] = '/';
		length = 1;
	}

	// At the position of each slash, store the distance from the previous slash.
	// Keep the initial slash untouched.
	// Make start point to the slash before beginning of the last path component.
	start = 0;
	for(index = 1; index < length; ++index)
	{
		if (path[index] == '/')
		{
			if ((index - start) > 255) return ERROR_UNSUPPORTED; // path component too long
			path[index] = index - start;
			start = index;
		}
	}

	// Normalize path by removing . and .. components and slash repetitions.
	offset = 0;
	do
	{
		if ((offset == raw_length) || (raw[offset] == '/')) // end of path component
		{
			switch (index - start)
			{
			case 1: // skip repeated /
				continue;

			case 2: // check for .
				if (raw[offset - 1] != '.') break;
				index = start + 1;
				continue;

			case 3: // check for ..
				if ((raw[offset - 2] != '.') || (raw[offset - 1] != '.')) break;
				if (start) start -= path[start]; // n the root directory .. points to the same directory
				index = start + 1;
				continue;
			}

			path[index] = index - start;
			start = index;
			if (offset == raw_length)
			{
				index += 1;
				break;
			}
		}
		else path[index] = raw[offset];

		index += 1;
		if (index == PATH_SIZE_LIMIT) return ERROR_UNSUPPORTED;
	} while (offset++ < raw_length);

	*path_length = index;

	// Restore path component separators to slashes.
	do
	{
		index = start;
		start -= path[index];
		path[index] = '/';
	} while (index);

	return 0;
}
