#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "base.h"
#include "path.h"
#include "test/path.h"

// TODO deprecated
char *normalize_(const char *restrict relative, size_t relative_length, size_t *restrict path_length)
{
	char *path, *temp;
	size_t length;

	size_t start, index;
	size_t offset;

	// Prepare the path for normalization.
	if (relative[0] != '/') // relative path
	{
		path = getcwd(0, 0);
		if (!path) abort();

		// Make sure the buffer will be able to store the whole path.
		path = realloc(path, PATH_SIZE_LIMIT);

		length = strlen(path);

		// At the position of each slash, store the distance from the previous slash.
		start = 0;
		for(index = 1; index < length; ++index)
		{
			if (path[index] == '/')
			{
				if ((index - start) > 255) return 0; // path component too long

				path[index] = index - start;
				start = index;
			}
		}

		// Put terminating slash if cwd is not the root directory
		if (length > 1)
		{
			path[length] = length - start;
			start = length;
			index = start + 1;
		}
	}
	else // absolute path
	{
		path = alloc(PATH_SIZE_LIMIT);

		//path[0] = '/';
		index = 0; // TODO not tested
	}

	// Normalize path.
	for(offset = 0; 1; ++offset)
	{
		if ((offset == relative_length) || (relative[offset] == '/')) // end of path component
		{
			switch (index - start)
			{
			case 1:
				if (offset == relative_length) goto finally; // TODO remove code duplication - this line is repeated 4 times
				continue; // skip repeated /

			case 2: // check for .
				if (relative[offset - 1] != '.') break;
				index -= 1;
				if (offset == relative_length) goto finally;
				continue;

			case 3: // check for ..
				if ((relative[offset - 2] == '.') && (relative[offset - 1] == '.'))
				{
					// .. in the root directory points to the same directory
					if (start) start -= path[start];
					index = start + 1;
					if (offset == relative_length) goto finally;
					continue;
				}
				break;
			}

			if (offset == relative_length) break;

			path[index] = index - start;
			start = index;
		}
		else path[index] = relative[offset];

		index += 1;
		if (index == PATH_SIZE_LIMIT)
		{
			free(path);
			return 0;
		}
	}

finally:

	length = index;

	// Restore the path component separators to slashes.
	do
	{
		index = start;
		start -= path[index];
		path[index] = '/';
	} while (index);

	// make sure the path ends with a /
	if (path[length - 1] != '/') path[length++] = '/';

	*path_length = length;
	return path;
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
