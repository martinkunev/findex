#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include "base.h"
#include "db.h"
#include "magic.h"
#include "array_string.h"

#define STRING(s) (s), sizeof(s) - 1

void details(const char *name, size_t name_length, const struct file *restrict file)
{
	char modified[64]; // TODO make sure this is big enough
	time_t mtime = file->mtime;
	ctime_r(&mtime, modified);

	struct string content = {0};

	if (file->content & CONTENT_DIRECTORY)
		if (string_append(&content, STRING("Directory, ")) < 0)
			abort();
    if (file->content & CONTENT_LINK)
		if (string_append(&content, STRING("Symbolic link, ")) < 0)
			abort();
    if (file->content & CONTENT_SPECIAL)
		if (string_append(&content, STRING("Special file, ")) < 0)
			abort();
    if (file->content & CONTENT_EXECUTABLE)
		if (string_append(&content, STRING("Executable, ")) < 0)
			abort();
    if (file->content & CONTENT_TEXT)
		if (string_append(&content, STRING("Text, ")) < 0)
			abort();
    if (file->content & CONTENT_ARCHIVE)
		if (string_append(&content, STRING("Archive, ")) < 0)
			abort();
    if (file->content & CONTENT_DOCUMENT)
		if (string_append(&content, STRING("Document, ")) < 0)
			abort();
    if (file->content & CONTENT_IMAGE)
		if (string_append(&content, STRING("Image, ")) < 0)
			abort();
    if (file->content & CONTENT_AUDIO)
		if (string_append(&content, STRING("Audio, ")) < 0)
			abort();
    if (file->content & CONTENT_VIDEO)
		if (string_append(&content, STRING("Video, ")) < 0)
			abort();
    if (file->content & CONTENT_DATABASE)
		if (string_append(&content, STRING("Database, ")) < 0)
			abort();

	// Ignore last comma and space.
	if (content.count)
		content.count -= 2;

	printf("File: %.*s\nSize: %u\nModify: %sContent: %.*s\n", name_length, name, file->size, modified, (int)content.count, content.data);
}
