#if defined(NDEBUG)
# define LEVEL Info
#else
# define LEVEL Debug
#endif

// TODO trace documentation
// The highest 2 bits of the length store next argument's type.

#define _logs(s, l, ...)	((0x3L << (sizeof(size_t) * 8 - 2)) | (l)), (s)
#define logs(...)			_logs(__VA_ARGS__, sizeof(__VA_ARGS__) - 1)
#define logi(i)				(0x2L << (sizeof(size_t) * 8 - 2)), ((int64_t)(i))

void trace(int fd, ...);
#define trace(...) (trace)(__VA_ARGS__, (size_t)0)

// Use these to find logging location.
#if 0
# define _line_str_ex(l) #l
# define _line_str(l) _line_str_ex(l)
# define LOCATION logs(__FILE__ ":" _line_str(__LINE__) " "),
#else
#define LOCATION
#endif

#if defined(NDEBUG)
# define debug(...)
#else
# define debug(...) trace(2, LOCATION __VA_ARGS__)
#endif
#define warning(...) trace(2, LOCATION __VA_ARGS__)
#define error(...) trace(2, LOCATION logs("\\e[1;31mError:\\e[0m "), __VA_ARGS__)
