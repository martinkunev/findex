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

struct db
{
	off_t data_offset;
	off_t index_offset;
	int data;
	int index;
};

struct search
{
	struct stat info;
	unsigned char *data_buffer;
};

struct file
{
	uint16_t path_length;
	uint16_t content;
	uint32_t mime_type;
	uint64_t mtime;
    uint64_t size;
} __attribute__((packed));

int db_new(struct db *restrict db);
int db_persist(struct db *restrict db);
void db_delete(struct db *restrict db);

int db_add(struct db *restrict db, const char *restrict path, size_t path_length, const struct file *restrict file);

int db_open(struct search *restrict search);
void db_close(const struct search *restrict search);

int db_find_fileinfo(struct file *restrict file, const char *restrict path, size_t length, const struct search *restrict search);
int db_set_fileinfo(struct file *restrict file, const char *restrict path, size_t path_length, const struct stat *restrict info);
