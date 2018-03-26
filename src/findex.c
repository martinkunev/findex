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
#include "arch.h"
#include "magic.h"
#include "path.h"
#include "findex.h"
#include "fs.h"

#define DB_ACCESS 0600

#define STRING(s) (s), sizeof(s) - 1

static int db_insert(int db, char *path, size_t path_length, const struct stat *restrict info, off_t *restrict offset)
{
	struct stat hardlink_info;
	struct file file = {0};
	uint16_t length = path_length;

	//fprintf(stderr, "\x1b[31m" "insert" "\x1b[0m %s\n", path);

	// If the file is a soft link, stat information about what it points to.
	if (S_ISLNK(info->st_mode))
	{
		if (stat(path, &hardlink_info) < 0)
			return 0; // TODO report this error; is it okay if I ignore the file?
		info = &hardlink_info;

		file.content |= CONTENT_LINK;
	}

	switch (info->st_mode & S_IFMT)
	{
		int entry;

	case S_IFDIR:
		file.content |= CONTENT_DIRECTORY;
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
				uint32_t type = (uint32_t)content(buffer, size);
				file.content |= typeinfo[type].content;
				endian_big32(&file.mime_type, &type);
			}
		}
		break;

	default:
		file.content |= CONTENT_SPECIAL;
		break;
	}

	file.content = htobe16(file.content);
	endian_big16(&file.path_length, &length);
	endian_big64(&file.mtime, &info->st_mtime);
	endian_big64(&file.size, &info->st_size);

	if (write(db, &file, sizeof(file)) < 0)
		return ERROR; // TODO
	if (write(db, path, path_length) < 0)
		return ERROR; // TODO

	// Add name index entry.
	//add_index(offset);
	//offset += sizeof(file) + path_length;

	// TODO this tests that the inode number from stat and directory listing are the same; why do I need this?
	//printf("%s\t%u\t%u\n", name, (unsigned)entry->d_ino, (unsigned)info.st_ino);

	return 0;
}

// Writes indexing data in a database.
static int db_index(int db, char *path, size_t path_length, off_t *restrict offset)
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

		status = db_insert(db, path, path_length, &info, offset);
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
			if (status = db_index(db, path, path_length, offset))
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
	char path[PATH_SIZE_LIMIT + 1];
	int db;

	size_t i;
	int status;

	off_t offset;

	if ((argc < 2) || ((argc == 2) && !strcmp(argv[1], "--help")))
	{
		write(2, STRING("Usage: ./findex <path> ...\n"));
		return ERROR_INPUT;
	}

	// TODO create parent directory?
	status = db_path_init(path);
	if (status)
		return status;
	db = fs_load(path, strlen(path), DB_ACCESS, 1);
	if (db < 0)
		return db;

	if (write(db, STRING(HEADER)) < 0)
		return ERROR;
	offset = sizeof(HEADER) - 1;

	for(i = 1; i < argc; i += 1)
	{
		char target[PATH_SIZE_LIMIT + 1];
		size_t target_length;

		status = normalize(target, &target_length, argv[i], strlen(argv[i]));
		if (status)
			goto error;

		status = db_index(db, target, target_length, &offset);
		if (status) goto error;
	}

	close(db);
	return 0;

error:
	close(db);
	unlink(path);
	return status;
}
