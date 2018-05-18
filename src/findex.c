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

#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__GLIBC__) && (((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 24)) || (__GLIBC__ > 2))
# define READDIR
#endif

#include "base.h"
#include "path.h"
#include "db.h"

#define STRING(s) (s), sizeof(s) - 1

static int db_insert(struct db *restrict db, const char *restrict path, size_t path_length, const struct stat *restrict info)
{
	struct file file;

	int status = db_set_fileinfo(&file, path, path_length, info);
	if (status < 0)
		return ((status == ERROR_CANCEL) ? 0 : status); // ERROR_CANCEL is non-fatal

	return db_add(db, path, path_length, &file);
}

// Writes indexing data in a database.
static int db_index(struct db *restrict db, char *path, size_t path_length)
{
	DIR *dir;
	struct dirent *entry;

	int status;

	// TODO better error handling

	dir = opendir(path);
	if (!dir)
	{
		switch (errno)
		{
		case EACCES:
			fprintf(stderr, "Permission denied to read %s\n", path);
			return 0;

		default:
			fprintf(stderr, "Unable to open %s\n", path);
			return ERROR;
		}
	}

#if !defined(READDIR)
	entry = malloc(offsetof(struct dirent, d_name) + pathconf(path, _PC_NAME_MAX) + 1);
	if (!entry)
	{
		closedir(dir);
		return ERROR_MEMORY;
	}
#endif

	path[path_length++] = '/';

	while (1)
	{
		const char *name;
		size_t name_length;
		struct stat info;

#if defined(READDIR)
		errno = 0;
		if (!(entry = readdir(dir)))
		{
			if (errno)
			{
				status = ERROR;
				goto finally;
			}
			else break; // no more entries
		}
#else
		struct dirent *more;

		if (readdir_r(dir, entry, &more))
		{
			status = ERROR;
			goto finally;
		}
		if (!more)
			break; // no more entries
#endif

		// skip . and ..
		if ((entry->d_name[0] == '.') && (!entry->d_name[1] || ((entry->d_name[1] == '.') && !entry->d_name[2])))
			continue;

		name = entry->d_name;
#if defined(_DIRENT_HAVE_D_NAMLEN)
		name_length = entry->d_namlen;
#else
		name_length = strlen(name);
#endif

		if (path_length + name_length + 1 > PATH_SIZE_LIMIT)
		{
			status = ERROR_UNSUPPORTED;
			goto finally;
		}
		memcpy(path + path_length, name, name_length);
		path_length += name_length;
		path[path_length] = 0;

		if (lstat(path, &info) < 0)
		{
			fprintf(stderr, "Unable to lstat %s\n", path);
			status = ERROR;
			goto finally;
		}

		if (status = db_insert(db, path, path_length, &info))
		{
			fprintf(stderr, "Unable to insert entry %s\n", path);
			goto finally;
		}

		// Recursively index each subdirectory.
		if (S_ISDIR(info.st_mode))
			if (status = db_index(db, path, path_length))
				goto finally;

		path_length -= name_length;
	}

	path[path_length -= 1] = 0;

	status = 0;

finally:
#if !defined(READDIR)
	free(entry);
#endif
	closedir(dir);

	return status;
}

int main(int argc, char *argv[])
{
	struct db db;

	size_t i;
	int status;

	if ((argc < 2) || ((argc == 2) && !strcmp(argv[1], "--help")))
	{
		write(2, STRING("Usage: findex <path> ...\n"));
		return ERROR_INPUT;
	}

	status = db_new(&db);
	if (status < 0)
		return status;

	for(i = 1; i < argc; i += 1)
	{
		char target[PATH_SIZE_LIMIT];
		size_t target_length;

		status = normalize(target, &target_length, argv[i], strlen(argv[i]));
		if (status)
			goto error;

		status = db_index(&db, target, target_length);
		if (status)
			goto error;
	}

	return -db_persist(&db);

error:
	db_delete(&db);
	return status;
}
