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
#include <errno.h>
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
#include "magic.h"
#include "db.h"

struct index_entry
{
	uint32_t hash;
	uint32_t start; // limits index database size to 4GiB
} __attribute__((packed));

#define HEAP_NAME heap_index
#define HEAP_TYPE struct index_entry
#define HEAP_ABOVE(a, b) ((a).hash >= (b).hash)
#include "generic/heap.g"

#define DB_ACCESS 0600
#define DB_HEADER "\x00\x03\x00\00\x00\x00\x00\x00"

#define DB_DATA_TEMPNAME "data_temp"
#define DB_INDEX_TEMPNAME "index_temp"

#define DB_DATA_NAME "data"
#define DB_INDEX_NAME "index"

#define INDEX_SIZE_LIMIT (64 * 1024 * 1024)

int db_new(struct db *restrict db)
{
	struct db temp;
	int status;

	struct path_buffer path_buffer;
	size_t length;

	// Initialize path for database files.
	status = path_init(&path_buffer);
	if (status < 0)
		return status;

	// Open data database and write header.
	length = path_set(&path_buffer, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
	temp.data = fs_load(path_buffer.data, length, DB_ACCESS, 1);
	if (temp.data < 0)
		return temp.data;
	if (write(temp.data, DB_HEADER, sizeof(DB_HEADER) - 1) < 0)
	{
		unlink(path_buffer.data);
		close(temp.data);
		return ERROR;
	}
	temp.data_offset = sizeof(DB_HEADER) - 1;

	// Open index database and write header.
	length = path_set(&path_buffer, DB_INDEX_TEMPNAME, sizeof(DB_INDEX_TEMPNAME) - 1);
	temp.index = fs_load(path_buffer.data, length, DB_ACCESS, 1);
	if (temp.index < 0)
	{
		path_set(&path_buffer, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
		unlink(path_buffer.data);
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

	// TODO support larger chunks by using merge sort on disk
	if (db->index_offset + sizeof(entry) > INDEX_SIZE_LIMIT)
		return ERROR; // TODO

	if (write(db->index, &entry, sizeof(entry)) < 0)
		return ERROR; // TODO

	db->index_offset += sizeof(entry);

	return 0;
}

int db_persist(struct db *restrict db)
{
	void *buffer;
	struct heap_index heap;

	struct path_buffer path_origin;
	struct path_buffer path_target;
	int status;

	close(db->data);

	// Map index file in memory.
	buffer = mmap(0, db->index_offset, PROT_WRITE, MAP_SHARED, db->index, 0);
	close(db->index);
	if (buffer == MAP_FAILED)
	{
		fprintf(stderr, "ERROR: mmap failed for index\n");
		return ERROR;
	}

	// Sort index with heap sort.
	heap.data = (void *)((char *)buffer + sizeof(DB_HEADER) - 1); // TODO ugly casting hack; think how to fix
	heap.count = (db->index_offset - sizeof(DB_HEADER) + 1) / sizeof(struct index_entry);
	heap_index_heapify(&heap);
	while (heap.count)
	{
		struct index_entry entry = heap.data[0];
		heap_index_pop(&heap);
		heap.data[heap.count] = entry;
	}

	munmap(buffer, db->index_offset);

	status = path_init(&path_origin);
	assert(status == 0);
	memcpy(path_target.data, path_origin.data, path_origin.prefix_length);
	path_target.prefix_length = path_origin.prefix_length;

	// Replace old data database with the new one.
	path_set(&path_origin, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
	path_set(&path_target, DB_DATA_NAME, sizeof(DB_DATA_NAME) - 1);
	if (rename(path_origin.data, path_target.data) < 0)
	{
		unlink(path_origin.data);
		path_set(&path_origin, DB_INDEX_TEMPNAME, sizeof(DB_INDEX_TEMPNAME) - 1);
		unlink(path_origin.data);

		return ERROR; // TODO error code
	}

	// Replace old index database with the new one.
	path_set(&path_origin, DB_INDEX_TEMPNAME, sizeof(DB_INDEX_TEMPNAME) - 1);
	path_set(&path_target, DB_INDEX_NAME, sizeof(DB_INDEX_NAME) - 1);
	if (rename(path_origin.data, path_target.data) < 0)
	{
		unlink(path_target.data); // cleanup outdated index
		// TODO issue warning that index wasn't created

		unlink(path_origin.data);
		path_set(&path_origin, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
		unlink(path_origin.data);

		return ERROR; // TODO error code
	}

	return 0;
}

void db_delete(struct db *restrict db)
{
	struct path_buffer buffer;
	int status;

	status = path_init(&buffer);
	assert(status == 0);

	path_set(&buffer, DB_DATA_TEMPNAME, sizeof(DB_DATA_TEMPNAME) - 1);
	unlink(buffer.data);
	close(db->data);

	path_set(&buffer, DB_INDEX_TEMPNAME, sizeof(DB_INDEX_TEMPNAME) - 1);
	unlink(buffer.data);
	close(db->index);
}

// TODO indicate error conditions
int db_open(struct search *restrict search)
{
	struct search temp;
	int status;
	int fd;
	void *buffer;

	struct path_buffer path_buffer;

	// Initialize path for database files.
	status = path_init(&path_buffer);
	if (status < 0)
		return status;

	path_set(&path_buffer, DB_DATA_NAME, sizeof(DB_DATA_NAME) - 1);
	fd = open(path_buffer.data, O_RDONLY);
	if (fd < 0)
		return ERROR;
	if (fstat(fd, &temp.info) < 0)
	{
		close(fd);
		return ERROR;
	}

	buffer = mmap(0, temp.info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (buffer == MAP_FAILED)
		return ERROR;
	temp.data_buffer = buffer;

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

static int update_match(struct file *restrict file, const unsigned char *buffer, const char *restrict path, size_t length)
{
	struct file temp;

	memcpy(&temp, buffer, sizeof(temp));

	if (temp.path_length != length)
		return ERROR_MISSING;

	if (memcmp(buffer + sizeof(temp), path, length))
		return ERROR_MISSING;

	memcpy(file, &temp, sizeof(temp));

	return 0;
}

int db_find_fileinfo(struct file *restrict file, const char *restrict path, size_t length, const struct search *restrict search)
{
	int fd;
	struct stat info;
	void *buffer;
	size_t index_size;

	uint32_t hashsum;
	struct index_entry *entries;

	int status;

	struct path_buffer path_buffer;

	status = path_init(&path_buffer);
	if (status < 0)
		return status;
	path_set(&path_buffer, DB_INDEX_NAME, sizeof(DB_INDEX_NAME) - 1);

	fd = open(path_buffer.data, O_RDONLY);
	if (fd < 0)
		return ERROR;
	if (fstat(fd, &info) < 0)
	{
		close(fd);
		return ERROR;
	}

	buffer = mmap(0, info.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	if (buffer == MAP_FAILED)
		return ERROR;

	index_size = info.st_size - sizeof(DB_HEADER) + 1;
	entries = (void *)((char *)buffer + sizeof(DB_HEADER) - 1); // TODO ugly casting hack; think how to fix
	hashsum = hash((const unsigned char *)path, length);

	// Use binary search to look for the path hash in the index.
	// Search the entries with the given hash until the actual path matches.
	{
		size_t low = 0, high = index_size / sizeof(*entries);
		size_t i;
		ssize_t i_offset;

		while (low < high)
		{
			i = (high - low) / 2 + low;

			if (entries[i].hash == hashsum)
				break;
			else if (entries[i].hash > hashsum)
				high = i;
			else // entries[i].hash < hashsum
				low = i + 1;
		}

		status = ERROR_MISSING;

		i_offset = 0;
		do
		{
			status = update_match(file, search->data_buffer + entries[i + i_offset].start, path, length);
			if (!status)
				goto finally;

			i_offset += 1;
		} while (entries[i + i_offset].hash == hashsum);

		i_offset = -1;
		while (entries[i + i_offset].hash == hashsum)
		{
			status = update_match(file, search->data_buffer + entries[i + i_offset].start, path, length);
			if (!status)
				goto finally;

			i_offset -= 1;
		}
	}

finally:
	munmap(buffer, info.st_size);
	return status;
}

int db_set_fileinfo(struct file *restrict file, const char *restrict path, size_t path_length, const struct stat *restrict info)
{
	struct stat hardlink_info;
	uint16_t length = path_length;

	*file = (struct file){0};

	// If the file is a soft link, stat information about what it points to.
	if (S_ISLNK(info->st_mode))
	{
		if (stat(path, &hardlink_info) < 0)
		{
			switch (errno)
			{
			case EACCES:
				fprintf(stderr, "WARNING: Cannot stat link target (permission denied) for %s\n", path);
				return ERROR_CANCEL;

			case ENAMETOOLONG:
				fprintf(stderr, "WARNING: Cannot stat link target (unsupported) for %s\n", path);
				return ERROR_CANCEL;

			case ENOENT:
			case ENOTDIR:
			case ELOOP:
				fprintf(stderr, "WARNING: Cannot stat link target (broken link) for %s\n", path);
				return ERROR_CANCEL;

			default:
				fprintf(stderr, "ERROR: Cannot stat link target for %s\n", path);
				return ERROR;

			case ENOMEM:
				return ERROR_MEMORY;
			}
		}
		info = &hardlink_info;

		file->content |= CONTENT_LINK;
	}

	switch (info->st_mode & S_IFMT)
	{
	case S_IFDIR:
		file->content |= CONTENT_DIRECTORY;
		break;

	case S_IFREG:
		{
			// TODO report open and read errors
			int entry = open(path, O_RDONLY);
			if (entry < 0)
				break;

			unsigned char buffer[MAGIC_SIZE];
			ssize_t size = read(entry, &buffer, MAGIC_SIZE);

			close(entry);

			if (size < 0)
				break;

			uint32_t type = (uint32_t)content(buffer, size);
			file->content |= typeinfo[type].content;
			file->mime_type = type;
		}
		break;

	default:
		file->content |= CONTENT_SPECIAL;
		break;
	}

	file->path_length = length;
	file->mtime = info->st_mtime;
	file->size = info->st_size;

	return 0;
}
