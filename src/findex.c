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
#include <fcntl.h>
#include <stddef.h>
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

static int db_insert(struct db *restrict db, char *path, size_t path_length, const struct stat *restrict info)
{
	struct file file;

	int status = db_file_info(&file, path, path_length, info);
	if (status < 0)
		return status;

	return db_add(db, path, path_length, &file);
}

// Writes indexing data in a database.
static int db_index(struct db *restrict db, char *path, size_t path_length)
{
	DIR *dir;
	struct dirent *entry;
#if !defined(READDIR)
	struct dirent *more;
#endif
	struct stat info;

	const char *name;
	size_t name_length;

	int status;

/* TODO I should be able to delete this
	if (path_length > PATH_SIZE_LIMIT)
	{
		status = ERROR_MEMORY;
		goto finally;
	}*/
	path[path_length] = 0;

	// TODO better error handling

	dir = opendir(path);
	if (!dir)
	{
		switch (errno)
		{
		case EACCES:
			return 0;

		default:
			return ERROR;
		}
	}

	entry = alloc(offsetof(struct dirent, d_name) + pathconf(path, _PC_NAME_MAX) + 1);

	while (1)
	{
#if defined(READDIR)
		errno = 0;
		if (!(entry = readdir(dir)))
		{
			if (errno)
				return ERROR;
			else
				break; // no more entries
		}
#else
		if (readdir_r(dir, entry, &more))
			return ERROR;
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

		if (path_length + name_length > PATH_SIZE_LIMIT)
		{
			status = ERROR_MEMORY;
			goto finally;
		}
		memcpy(path + path_length, name, name_length);
		path_length += name_length;
		path[path_length] = 0;

		if (lstat(path, &info) < 0) return ERROR;

		status = db_insert(db, path, path_length, &info);
		if (status)
			return status;

		// Recursively index each directory.
		if (S_ISDIR(info.st_mode))
		{
			if (path_length + 1 > PATH_SIZE_LIMIT)
			{
				status = ERROR_MEMORY;
				goto finally;
			}
			path[path_length++] = '/';
			if (status = db_index(db, path, path_length))
				goto finally;
			path_length -= 1;
		}

		path_length -= name_length;
	}

	status = 0;

finally:
	closedir(dir);
#if !defined(READDIR)
	free(entry);
#endif

	return status;
}

int main(int argc, char *argv[])
{
	struct db db;

	size_t i;
	int status;

	if ((argc < 2) || ((argc == 2) && !strcmp(argv[1], "--help")))
	{
		write(2, STRING("Usage: ./findex <path> ...\n"));
		return ERROR_INPUT;
	}

	status = db_new(&db);
	if (status < 0)
		return status;

	for(i = 1; i < argc; i += 1)
	{
		char target[PATH_SIZE_LIMIT + 1];
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
