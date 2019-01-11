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

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//#include <stddef.h>

#include "base.h"
#include "path.h"
#include "magic.h"
#include "findex.h"

static int detect(char *path, size_t path_length, struct file *restrict file)
{
	struct stat info;
	uint16_t length = path_length;

	if (lstat(path, &info) < 0) return -1; // TODO error

	// If the file is a soft link, stat information about what it points to.
	if (S_ISLNK(info.st_mode))
	{
		if (stat(path, &info) < 0)
			return 0; // TODO report this error; is it okay if I ignore the file?

		file->content |= CONTENT_LINK;
	}

	switch (info.st_mode & S_IFMT)
	{
		int entry;

	case S_IFDIR:
		file->content |= CONTENT_DIRECTORY;
		break;

	case S_IFREG:
		// TODO report open and read errors
		entry = open(path, O_RDONLY);
		if (entry >= 0)
		{
			unsigned char buffer[MAGIC_SIZE];
			ssize_t size = read(entry, &buffer, MAGIC_SIZE);

			close(entry);

			if (size >= 0)
			{
				enum type type = content(buffer, size);
				file->content |= typeinfo[type].content;
				file->mime_type = type;
			}
		}
		break;

	default:
		file->content |= CONTENT_SPECIAL;
		break;
	}

	memcpy(&file->path_length, &length, sizeof(file->path_length));
	memcpy(&file->mtime, &info.st_mtime, sizeof(file->mtime));
	memcpy(&file->size, &info.st_size, sizeof(file->size));

	return 0;
}

#include <stdio.h>

int main(int argc, char *argv[])
{
	size_t i;

	for(i = 1; i < argc; i += 1)
	{
		char target[PATH_SIZE_LIMIT];
		size_t target_length;
		int status;
		struct file file;

		status = normalize(target, &target_length, argv[i], strlen(argv[i]));
		if (status)
			continue; // TODO error
		target[target_length -= 1] = 0;

		detect(target, target_length, &file);

		printf("%s: %s\n", target, typeinfo[file.mime_type].mime_type->data);
	}

	return 0;
}
