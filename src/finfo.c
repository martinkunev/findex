#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "base.h"
#include "path.h"
#include "db.h"
#include "magic.h"
#include "string.h"

#define STRING(s) (s), sizeof(s) - 1

static int usage(int code)
{
	write(1, STRING(
"Usage: finfo <file> ...\n"
	));
	return code;
}

static void print(const char *name, size_t name_length, const struct file *restrict file)
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

	printf("File: %s\nSize: %u\nModify: %sContent: %.*s\n", name, file->size, modified, (int)content.count, content.data);
}

int main(int argc, char *argv[])
{
	int i;

	// TODO better error handling

	if ((argc < 2) || !strcmp(argv[1], "--help"))
		return usage(0);

	for(i = 1; i < argc; i += 1)
	{
		int status;
		struct stat info;
		size_t length;
		struct file file;

		if (stat(argv[i], &info) < 0)
			return ERROR;

		length = strlen(argv[i]);
		status = db_file_info(&file, argv[i], length, &info);
		if (status < 0)
			return -status;

		if (i > 1)
			putc('\n', stdout); // separate output by new lines
		print(argv[i], length, &file);
	}

	return 0;
}
