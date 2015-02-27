#include <dirent.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base.h"
#include "arch.h"
#include "buffer.h"
#include "magic.h"
#include "path.h"
#include "findex.h"

#define DB_PATH "/.cache/filement" /* database path relative to the user's home directory */

#define STRING(s) (s), sizeof(s) - 1

// TODO i don't follow symbolic links; is this okay?

static int db_path_init(char path[static PATH_SIZE_LIMIT])
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

static int db_insert(int db, char *path, size_t path_length, const struct stat *restrict info)
{
	struct file file;
	uint16_t length = path_length;

	endian_big16(&file.mode, &info->st_mode); // TODO is this right?
	endian_big16(&file.path_length, &length);
	endian_big64(&file.mtime, &info->st_mtime);
	endian_big64(&file.size, &info->st_size);

	{
		ssize_t size;
		char buffer[MAGIC_SIZE];
		int entry;

		entry = open(path, O_RDONLY);
		if (entry >= 0)
		{
			size = read(entry, &buffer, MAGIC_SIZE);
			close(entry);

			if (size >= 0)
			{
				uint32_t file_content = content(buffer, size);
				endian_big32(&file.content, &file_content);
			}
			else file.content = 0;
		}
		else file.content = 0;
	}

	if (write(db, &file, sizeof(file)) < 0) return -1; // TODO
	if (write(db, path, path_length) < 0) return -1; // TODO

	// TODO this tests that the inode number from stat and directory listing are the same; why do I need this?
	//printf("%s\t%u\t%u\n", name, (unsigned)entry->d_ino, (unsigned)info.st_ino);

	return 0;
}

// Writes indexing data in a database.
static int db_index(int db, char *path, size_t path_length)
{
	DIR *dir;
	struct dirent *entry, *more;
	struct stat info;

	const char *name;
	size_t name_length;

	int status;

	entry = alloc(offsetof(struct dirent, d_name) + pathconf(path, _PC_NAME_MAX) + 1);

	if (path_length + 1 > PATH_SIZE_LIMIT)
	{
		status = ERROR_MEMORY;
		goto finally;
	}
	path[path_length] = 0;

	dir = opendir(path);
	if (!dir) return -1; // TODO better error

	while (1)
	{
		if (readdir_r(dir, entry, &more)) return -1; // TODO error
		if (!more) break; // no more entries

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
			status = ERROR_MEMORY;
			goto finally;
		}
		memcpy(path + path_length, name, name_length);
		path_length += name_length;
		path[path_length] = 0;

		if (lstat(path, &info) < 0) return -1; // TODO error

		mode_t mode = info.st_mode & S_IFMT;

		if ((mode == S_IFDIR) || (mode == S_IFREG))
			if (db_insert(db, path, path_length, &info))
				return -1; // TODO error

		// Recursively index each directory.
		if (mode == S_IFDIR)
		{
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
	free(entry);

	return status;
}

int main(int argc, char *argv[])
{
	char path[PATH_SIZE_LIMIT];
	int db;

	size_t i;
	int status;

	if (argc < 2)
	{
		write(2, STRING("Usage: ./findex <path> ...\n"));
		return ERROR_INPUT;
	}

	status = db_path_init(path);
	if (status) return status;
	db = creat(path, 0600);
	if (db < 0) return db; // TODO better error checking

	write(db, STRING(HEADER));

	for(i = 1; i < argc; ++i)
	{
		size_t path_length;
		char *restrict path = normalize(argv[i], strlen(argv[i]), &path_length);
		if (!path)
		{
			status = ERROR_MEMORY;
			goto error;
		}

		status = db_index(db, path, path_length);
		free(path);
		if (status) goto error;
	}

	close(db);
	return 0;

error:
	close(db);
	unlink(path);
	return status;
}
