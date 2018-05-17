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

enum {PATH_SIZE_LIMIT = 4096};

struct path_buffer
{
	size_t prefix_length;
	char data[PATH_SIZE_LIMIT];
};

int path_init(struct path_buffer *restrict path);
size_t path_set(struct path_buffer *restrict path, const char *restrict name, size_t name_length);

int normalize(char path[static restrict PATH_SIZE_LIMIT], size_t *restrict path_length, const char *restrict raw, size_t raw_length);
