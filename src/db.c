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
#include <fcntl.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "base.h"
#include "path.h"
#include "fs.h"
#include "hash.h"
#include "db.h"

#define DB_ACCESS 0600
#define DB_HEADER "\x00\x03\x00\00\x00\x00\x00\x00"

#define DB_DATA_TEMPNAME "data_temp"
#define DB_INDEX_TEMPNAME "index_temp"

#define DB_DATA_NAME "data"
#define DB_INDEX_NAME "index"

#define PATH_PREFIX "/.cache/filement/" /* relative path to directory storing database */

struct index_entry
{
	uint32_t hash;
	uint32_t start; // limits index database size to 4GiB
};

static size_t path_generate(char path[static PATH_SIZE_LIMIT + 1], const char *restrict name, size_t name_length)
{
	const char *home;
	size_t length;

	home = getenv("HOME");
	if (!home)
		return 0;

	length = strlen(home);
	if ((length + sizeof(PATH_PREFIX) - 1 + name_length + 1) > PATH_SIZE_LIMIT)
		return 0;

	memcpy(path, home, length);

	memcpy(path + length, PATH_PREFIX, sizeof(PATH_PREFIX) - 1);
	length += sizeof(PATH_PREFIX) - 1;

	memcpy(path + length, name, name_length);
	length += name_length;

	path[length] = 0;

	return length;
}

static size_t path_change(char dir[static PATH_SIZE_LIMIT + 1], size_t dir_length, const char *restrict name, size_t name_length)
{
	size_t length;
	memcpy(dir + dir_length, name, name_length);
	length = dir_length + name_length;
	dir[length] = 0;
	return length;
}

int db_new(struct db *restrict db)
{
	struct db temp;

	// Generate paths to database files.
	temp.data_path_length = path_generate(temp.data_path, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
	if (!temp.data_path_length)
		return ERROR;
	temp.index_path_length = path_generate(temp.index_path, DB_INDEX_TEMPNAME, sizeof(DB_INDEX_TEMPNAME) - 1);
	if (!temp.index_path_length)
		return ERROR;

	// Open data database and write header.
	temp.data = fs_load(temp.data_path, temp.data_path_length, DB_ACCESS, 1);
	if (temp.data < 0)
		return temp.data;
	if (write(temp.data, DB_HEADER, sizeof(DB_HEADER) - 1) < 0)
	{
		unlink(temp.data_path);
		close(temp.data);
		return ERROR;
	}
	temp.data_offset = sizeof(DB_HEADER) - 1;

	// Open index database and write header.
	temp.index = fs_load(temp.index_path, temp.index_path_length, DB_ACCESS, 1);
	if (temp.index < 0)
	{
		unlink(temp.data_path);
		close(temp.data);
		return temp.index;
	}
	if (write(temp.index, DB_HEADER, sizeof(DB_HEADER) - 1) < 0)
	{
		db_delete(&temp);
		return ERROR;
	}
	temp.index_offset = sizeof(DB_HEADER) - 1;

	*db = temp;
	return 0;
}

int db_persist(struct db *restrict db)
{
	char path[PATH_SIZE_LIMIT + 1];

	// Replace old data database with the new one.
	memcpy(path, db->data_path, db->data_path_length);
	assert(sizeof(DB_DATA_NAME) < sizeof(DB_DATA_TEMPNAME)); // ensures the following operation will succeed
	path_change(path, db->data_path_length - sizeof(DB_DATA_TEMPNAME) + 1, DB_DATA_NAME, sizeof(DB_DATA_NAME) - 1);
	if (rename(db->data_path, path) < 0)
		goto error;

	// Replace old index database with the new one.
	memcpy(path, db->index_path, db->index_path_length);
	assert(sizeof(DB_INDEX_NAME) < sizeof(DB_INDEX_TEMPNAME)); // ensures the following operation will succeed
	path_change(path, db->index_path_length - sizeof(DB_INDEX_TEMPNAME) + 1, DB_INDEX_NAME, sizeof(DB_INDEX_NAME) - 1);
	if (rename(db->index_path, path) < 0)
		goto error; // TODO we can possibly have index not corresponding to the database here. handle this

	close(db->data);
	close(db->index);

	return 0;

error:
	db_delete(db);
	return ERROR;
}

void db_delete(struct db *restrict db)
{
	unlink(db->data_path);
	unlink(db->index_path);
	close(db->data);
	close(db->index);
}

int db_add(struct db *restrict db, const char *restrict path, size_t path_length, const struct file *restrict file)
{
	struct index_entry entry;

	if (write(db->data, file, sizeof(*file)) < 0)
		return ERROR; // TODO
	if (write(db->data, path, path_length) < 0)
		return ERROR; // TODO

	entry.hash = hash((const unsigned char *)path, path_length);
	entry.start = db->data_offset;

	db->data_offset += sizeof(*file) + path_length;

	// TODO handle entry

	return 0;
}

// TODO indicate error conditions
int db_open(struct search *restrict search)
{
	struct search temp;
	int fd;

	// Generate paths to database files.
	temp.data_path_length = path_generate(temp.data_path, DB_DATA_NAME, sizeof(DB_DATA_NAME) - 1);
	if (!temp.data_path_length)
		return ERROR;
	temp.index_path_length = path_generate(temp.index_path, DB_INDEX_NAME, sizeof(DB_INDEX_NAME) - 1);
	if (!temp.index_path_length)
		return ERROR;

	// Open data database and write header.
	fd = open(temp.data_path, O_RDONLY);
	if (fd < 0)
		return ERROR;
	if (fstat(fd, &temp.info) < 0)
	{
		close(fd);
		return ERROR;
	}

	temp.data_buffer = mmap(0, temp.info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (temp.data_buffer == MAP_FAILED)
		return ERROR;

	// Check file header.
	if (temp.info.st_size < (sizeof(DB_HEADER) - 1))
		goto error; // unexpected EOF
	if (memcmp(temp.data_buffer, DB_HEADER, sizeof(DB_HEADER) - 1))
		goto error; // invalid database format

	*search = temp;
	return 0;

error:
	db_close(&temp);
	return ERROR_INPUT;
}

void db_close(const struct search *restrict search)
{
	munmap(search->data_buffer, search->info.st_size);
}
