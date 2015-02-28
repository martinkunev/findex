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

static inline void *realloc_(void *old, size_t size)
{
	void *new = (realloc)(old, size);
	if (!new)
	{
		free(old);
		abort();
	}
	return new;
}
#define realloc(buffer, size) realloc_((buffer), (size))

static inline void *dupalloc(void *old, size_t size)
{
	void *new = malloc(size);
	if (!new) abort();
	memcpy(new, old, size);
	return new;
}

////////////////

typedef struct
{
	size_t size;
	unsigned char data[];
} bytes_t;

#define bytes_t(n) struct \
	{ \
		size_t size; \
		char data[n]; \
	}

#define bytes(value) {sizeof(value) - 1, value}

#define bytes_define(variable, value) bytes_t(sizeof(value) - 1) variable = bytes(value)

// TODO ? make static assert for offsetof(..., data) as this ensures the struct is compatible with bytes_t
#define bytes_p(s) (bytes_t *)&( \
		struct \
		{ \
			size_t size; \
			char data[sizeof(s) - 1]; \
		} \
	){sizeof(s) - 1, (s)}
