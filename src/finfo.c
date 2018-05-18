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
#include "details.h"
#include "array_string.h"

#define STRING(s) (s), sizeof(s) - 1

static int usage(int code)
{
	write(1, STRING(
"Usage: finfo <file> ...\n"
	));
	return code;
}

int main(int argc, char *argv[])
{
	int i;
	int status;

	struct search search;

	// TODO better error handling

	if ((argc < 2) || !strcmp(argv[1], "--help"))
		return usage(0);

	status = db_open(&search);
	if (status < 0)
		return -status;

	for(i = 1; i < argc; i += 1)
	{
		int status;
		//struct stat info;
		size_t length;
		struct file file;

		char path[PATH_SIZE_LIMIT + 1];

		length = strlen(argv[i]);

		status = normalize(path, &length, argv[i], length);
		if (status < 0)
			break;

		/*if (stat(argv[i], &info) < 0)
		{
			status = ERROR;
			break;
		}*/

		//status = db_set_fileinfo(&file, path, length, &info);
		status = db_find_fileinfo(&file, path, length, &search);
		if (status < 0)
			break;

		if (i > 1)
			putc('\n', stdout); // separate output by new lines
		details(argv[i], length, &file);
	}

	db_close(&search);

	return -status;
}
