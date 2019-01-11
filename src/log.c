#include <stdarg.h>
#include <time.h>
#include <unistd.h>

#include "types.h"
#include "format.h"

#define trace_type(l) (((l) >> (sizeof(size_t) * 8 - 2)) & 0x1L)
#define trace_length(l) ((l) & ~(0x3L << (sizeof(size_t) * 8 - 2)))

// WARNING: Trace output that is too long will be truncated.
// WARNING: C99 limits trace arguments to 63 ((127 - 1) / 2).
void trace(int fd, ...)
{
	va_list buffers;
	size_t length;
	int64_t integer;
	char *string;

	char buffer[1024], *start = buffer;

	static const char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	struct tm info;
	time_t timestamp = time(0);
	gmtime_r(&timestamp, &info);

	// [yyyy MON dd hh:mm:ss]
	// The buffer is always big enough to hold date.
	*start++ = '[';
	start = format_uint(start, info.tm_mday, 10, 2, '0');
	*start++ = ' ';
	start = format_bytes(start, months[info.tm_mon], 3);
	*start++ = ' ';
	start = format_uint(start, info.tm_year + 1900, 10, 4, '0');
	*start++ = ' ';
	start = format_uint(start, info.tm_hour, 10, 2, '0');
	*start++ = ':';
	start = format_uint(start, info.tm_min, 10, 2, '0');
	*start++ = ':';
	start = format_uint(start, info.tm_sec, 10, 2, '0');
	*start++ = ']';
	*start++ = ' ';

	// Handle each buffer passed as an argument.
	va_start(buffers, fd);
	while (length = va_arg(buffers, size_t))
	{
		if (trace_type(length)) // string
		{
			size_t entry_length = trace_length(length);
			if ((start + entry_length - buffer) >= sizeof(buffer))
				break; // skip the rest of the arguments to prevent buffer overflow

			string = va_arg(buffers, char *);
			start = format_bytes(start, string, entry_length);
		}
		else // integer
		{
			if ((start + 20 - buffer) >= sizeof(buffer)) // any int64_t can be stored in 20B in base-10
				break; // skip the rest of the arguments to prevent buffer overflow

			integer = va_arg(buffers, int64_t);
			start = format_int(start, integer, 10);
		}
	}
	va_end(buffers);

	*start++ = '\n';

	write(fd, buffer, start - buffer);
}
