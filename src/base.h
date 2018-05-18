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

// System resources are not sufficient to handle the request.
#define ERROR_MEMORY				-1

// Invalid input data.
#define ERROR_INPUT					-2

// Request requires access rights that are not available.
#define ERROR_ACCESS				-3

// Entity that is required for the operation is missing.
#define ERROR_MISSING				-4

// Unable to create a necessary entity because it exists.
#define ERROR_EXIST					-5

// Filement filesystem internal error.
#define ERROR_EVFS					-6

// Temporary condition caused error. TODO maybe rename this to ERROR_BUSY
#define ERROR_AGAIN					-7

// Unsupported feature is required to satisfy the request.
#define ERROR_UNSUPPORTED			-8 /* TODO maybe rename to ERROR_SUPPORT */

// Read error.
#define ERROR_READ					-9

// Write error.
#define ERROR_WRITE					-10

// Action was cancelled.
#define ERROR_CANCEL				-11

// An asynchronous operation is now in progress.
#define ERROR_PROGRESS				-12

// Unable to resolve domain.
#define ERROR_RESOLVE				-13

// Network operation failed.
#define ERROR_NETWORK				-14

// An upstream server returned invalid response.
#define ERROR_GATEWAY				-15

// Invalid session.
#define ERROR_SESSION				-16

// Unknown error.
#define ERROR						-32767

////////////////

static inline void *alloc(size_t size)
{
	void *buffer = malloc(size);
	if (!buffer) abort();
	return buffer;
}

static inline void *dupalloc(void *old, size_t size)
{
	void *new = malloc(size);
	if (!new) abort();
	memcpy(new, old, size);
	return new;
}

////////////////

#include <stddef.h>
#include <stdio.h>

// WARNING: This code requires compiler support for __attribute__((aligned())).

//#define STATIC_ASSERT(predicate, message) extern int assert_##message[predicate ? 1 : -1];

struct bytes
{
	size_t size;
	unsigned char data[];
} __attribute__((aligned(1)));

#define bytes(init) &((union {struct {size_t size; unsigned char data[sizeof(init)];} __attribute__((aligned(1))) _; struct bytes bytes;}){sizeof(init) - 1, init}).bytes
